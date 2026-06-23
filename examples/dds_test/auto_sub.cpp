// 자동 시나리오 러너 — 팔로워 (RPI5 = Subscriber 측)
// RPI4(auto_pub)가 제어 토픽으로 보내는 명령을 받아 해당 QoS로 DataReader를 만들고,
// 수신 개수를 세어 결과를 RPI4로 회신한다.
#include "HelloWorldPubSubTypes.hpp"

#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/topic/ContentFilteredTopic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace eprosima::fastdds::dds;

static std::string field(const std::string& s, const std::string& key)
{
    auto p = s.find(key + "=");
    if (p == std::string::npos) return "";
    p += key.size() + 1;
    auto e = s.find('|', p);
    return s.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
}

class CountListener : public DataReaderListener
{
public:
    std::atomic<int> received{0};
    std::atomic<int> matched{0};
    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override
    {
        matched = info.current_count;
    }
    void on_data_available(DataReader* reader) override
    {
        HelloWorld s; SampleInfo i;
        while (reader->take_next_sample(&s, &i) == RETCODE_OK)
            if (i.valid_data) ++received;
    }
};

int main(int argc, char** argv)
{
    int domain = 0;
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--domain" && i + 1 < argc) domain = std::atoi(argv[++i]);

    auto* factory = DomainParticipantFactory::get_instance();
    DomainParticipant* part = factory->create_participant(domain, PARTICIPANT_QOS_DEFAULT);
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(part);

    // ---- 제어 토픽 (항상 호환되는 고정 QoS: reliable + transient_local) ----
    Topic* ctrlTopic = part->create_topic("DDSTestControl", type.get_type_name(), TOPIC_QOS_DEFAULT);
    Publisher* cpub = part->create_publisher(PUBLISHER_QOS_DEFAULT);
    Subscriber* csub = part->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    DataWriterQos cwq = DATAWRITER_QOS_DEFAULT;
    cwq.reliability().kind = RELIABLE_RELIABILITY_QOS;
    cwq.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    cwq.history().kind = KEEP_LAST_HISTORY_QOS; cwq.history().depth = 100;
    DataWriter* cw = cpub->create_datawriter(ctrlTopic, cwq);
    DataReaderQos crq = DATAREADER_QOS_DEFAULT;
    crq.reliability().kind = RELIABLE_RELIABILITY_QOS;
    crq.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    crq.history().kind = KEEP_LAST_HISTORY_QOS; crq.history().depth = 100;
    DataReader* cr = csub->create_datareader(ctrlTopic, crq);

    // ---- 데이터 토픽 (시나리오별 QoS로 Reader를 매번 새로 생성) ----
    Topic* dataTopic = part->create_topic("HelloWorldTopic", type.get_type_name(), TOPIC_QOS_DEFAULT);
    Subscriber* dsub = part->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    DataReader* dr = nullptr;
    ContentFilteredTopic* cft = nullptr;
    CountListener listener;

    auto sendCtrl = [&](int idx, const std::string& msg) {
        HelloWorld m; m.index(idx); m.message(msg); cw->write(&m);
    };

    std::cout << "[SUB-RUNNER] 시작. RPI4(auto_pub)의 명령 대기 중... (domain=" << domain << ")" << std::endl;

    int step = 0;
    bool running = true;
    while (running)
    {
        HelloWorld cmd; SampleInfo si;
        bool got = false;
        while (cr->take_next_sample(&cmd, &si) == RETCODE_OK)
        {
            if (!si.valid_data) continue;
            const std::string& msg = cmd.message();
            if (msg.rfind("CMD:", 0) != 0) continue;  // 내가 보낸 ACK 등은 무시
            got = true;

            if (msg.find("CMD:QUIT") == 0)
            {
                std::cout << "[SUB-RUNNER] QUIT 수신. 종료." << std::endl;
                running = false;
                break;
            }
            else if (msg.find("CMD:RUN") == 0)
            {
                std::string name = field(msg, "name");
                std::string rel = field(msg, "rel");
                std::string dur = field(msg, "dur");
                std::string his = field(msg, "his");
                int depth = std::atoi(field(msg, "depth").c_str());
                std::string part_name = field(msg, "part");    // Partition QoS
                std::string filter = field(msg, "filter");     // Content filter 식

                DataReaderQos q = DATAREADER_QOS_DEFAULT;
                q.reliability().kind = (rel == "re") ? RELIABLE_RELIABILITY_QOS : BEST_EFFORT_RELIABILITY_QOS;
                q.durability().kind = (dur == "tl") ? TRANSIENT_LOCAL_DURABILITY_QOS : VOLATILE_DURABILITY_QOS;
                q.history().kind = (his == "ka") ? KEEP_ALL_HISTORY_QOS : KEEP_LAST_HISTORY_QOS;
                if (depth > 0) q.history().depth = depth;

                // Partition QoS: Subscriber 단위. 매 시나리오마다 재설정(없으면 기본 파티션)
                SubscriberQos sqos;
                dsub->get_qos(sqos);
                sqos.partition().clear();
                if (!part_name.empty()) sqos.partition().push_back(part_name.c_str());
                dsub->set_qos(sqos);

                listener.received = 0; listener.matched = 0;
                if (!filter.empty())
                {
                    // Content Filtered Topic: 내용 기반 필터링 (예: index > 5)
                    cft = part->create_contentfilteredtopic("CFT_" + name, dataTopic, filter, {});
                    dr = dsub->create_datareader(cft, q, &listener);
                }
                else
                {
                    dr = dsub->create_datareader(dataTopic, q, &listener);
                }
                std::cout << "[SUB-RUNNER] [" << name << "] Reader 생성 (rel=" << rel
                          << " dur=" << dur << " his=" << his << " depth=" << depth
                          << " part=" << (part_name.empty() ? "-" : part_name)
                          << " filter=" << (filter.empty() ? "-" : filter) << ") → READY" << std::endl;
                sendCtrl(++step, "ACK:READY|name=" + name);
            }
            else if (msg.find("CMD:STOP") == 0)
            {
                std::string name = field(msg, "name");
                int cnt = listener.received.load();
                std::cout << "[SUB-RUNNER] [" << name << "] STOP. 수신=" << cnt << " → 결과 회신" << std::endl;
                sendCtrl(++step, "ACK:RES|name=" + name + "|count=" + std::to_string(cnt));
                if (dr) { dsub->delete_datareader(dr); dr = nullptr; }
                if (cft) { part->delete_contentfilteredtopic(cft); cft = nullptr; }
            }
        }
        if (!got) std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    factory->delete_participant(part);
    return 0;
}
