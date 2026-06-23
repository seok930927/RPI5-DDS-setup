// 자동 시나리오 러너 — 오케스트레이터 (RPI4 = Publisher 측)
// 정해진 시나리오 목록을 순서대로 자동 진행한다.
// 각 시나리오: 제어 토픽으로 RPI5에 "RUN(QoS)" 명령 → 데이터 송신 → "STOP" → 결과 수신.
#include "HelloWorldPubSubTypes.hpp"

#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace eprosima::fastdds::dds;

enum Check { EXACT, ZERO, NONZERO };

struct Scenario
{
    std::string name;
    std::string desc;
    std::string pub_rel, sub_rel;  // "re" / "be"
    std::string pub_dur, sub_dur;  // "v"  / "tl"
    std::string his;               // "kl" / "ka"
    int depth;
    int samples;
    bool late;                     // 늦은 조인 (Pub 먼저 송신 후 Sub 조인)
    int expected;
    Check check;
    std::string pub_part = "";     // Partition QoS (Publisher측)
    std::string sub_part = "";     // Partition QoS (Subscriber측)
    std::string filter = "";       // Content filter 식 (Subscriber측, 예: "index > 5")
};

static std::string field(const std::string& s, const std::string& key)
{
    auto p = s.find(key + "=");
    if (p == std::string::npos) return "";
    p += key.size() + 1;
    auto e = s.find('|', p);
    return s.substr(p, (e == std::string::npos) ? std::string::npos : e - p);
}

static ReliabilityQosPolicyKind relOf(const std::string& v)
{ return (v == "re") ? RELIABLE_RELIABILITY_QOS : BEST_EFFORT_RELIABILITY_QOS; }
static DurabilityQosPolicyKind durOf(const std::string& v)
{ return (v == "tl") ? TRANSIENT_LOCAL_DURABILITY_QOS : VOLATILE_DURABILITY_QOS; }
static HistoryQosPolicyKind hisOf(const std::string& v)
{ return (v == "ka") ? KEEP_ALL_HISTORY_QOS : KEEP_LAST_HISTORY_QOS; }

class CtrlListener : public DataReaderListener {};

int main(int argc, char** argv)
{
    int domain = 0;
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--domain" && i + 1 < argc) domain = std::atoi(argv[++i]);

    // ---- 시나리오 목록 ----
    std::vector<Scenario> scen = {
        {"1-1/2-1", "기본통신·Discovery",        "re","re", "v","v",  "kl",10, 10, false, 10, EXACT},
        {"2-2",     "HEARTBEAT/ACKNACK(RELIABLE)","re","re", "v","v",  "kl",10, 10, false, 10, EXACT},
        {"3-1-BE",  "Reliability BEST_EFFORT",   "be","be", "v","v",  "kl",10, 10, false, 10, NONZERO},
        {"3-1-RE",  "Reliability RELIABLE",      "re","re", "v","v",  "kl",10, 10, false, 10, EXACT},
        {"3-2-VOL", "Durability VOLATILE(늦은조인)","re","re","v","v",  "kl",10, 10, true,  0,  ZERO},
        {"3-2-TL",  "Durability TRANSIENT_LOCAL(늦은조인)","re","re","tl","tl","kl",10,10,true,10,EXACT},
        {"3-3-KL5", "History KEEP_LAST depth=5(늦은조인)","re","re","tl","tl","kl",5, 20, true, 5, EXACT},
        {"3-3-KA",  "History KEEP_ALL(늦은조인)", "re","re", "tl","tl", "ka",10,20, true, 20, EXACT},
        {"3-4-a",   "Mismatch Pub=BE/Sub=RE",    "be","re", "v","v",  "kl",10, 10, false, 0,  ZERO},
        {"3-4-b",   "Mismatch Pub=VOL/Sub=TL",   "re","re", "v","tl", "kl",10, 10, false, 0,  ZERO},
        {"3-4-c",   "Compatible Pub=RE/Sub=BE",  "re","be", "v","v",  "kl",10, 10, false, 10, NONZERO},
        // ---- DDS 고유 기능 보강 ----
        {"4-1-PART-OK","Partition 일치(A=A)",    "re","re", "v","v",  "kl",10, 10, false, 10, NONZERO, "A","A",""},
        {"4-1-PART-NO","Partition 불일치(A!=B)", "re","re", "v","v",  "kl",10, 10, false, 0,  ZERO,    "A","B",""},
        {"4-2-CFT",    "ContentFilter index>5",  "re","re", "v","v",  "kl",10, 10, false, 5,  EXACT,   "","","index > 5"},
    };

    auto* factory = DomainParticipantFactory::get_instance();
    DomainParticipant* part = factory->create_participant(domain, PARTICIPANT_QOS_DEFAULT);
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(part);

    // ---- 제어 토픽 (RPI5와 동일한 고정 QoS) ----
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

    // ---- 데이터 토픽 ----
    Topic* dataTopic = part->create_topic("HelloWorldTopic", type.get_type_name(), TOPIC_QOS_DEFAULT);
    Publisher* dpub = part->create_publisher(PUBLISHER_QOS_DEFAULT);

    int step = 0;
    auto sendCtrl = [&](const std::string& msg) {
        HelloWorld m; m.index(++step); m.message(msg); cw->write(&m);
    };

    // ACK:<kind> 대기 (name 일치). 반환: 매칭 메시지 전체("" = 타임아웃)
    auto waitAck = [&](const std::string& kind, const std::string& name, int timeout_sec) -> std::string {
        int ticks = timeout_sec * 20;
        for (int t = 0; t < ticks; ++t)
        {
            HelloWorld a; SampleInfo si;
            while (cr->take_next_sample(&a, &si) == RETCODE_OK)
            {
                if (!si.valid_data) continue;
                const std::string& m = a.message();
                if (m.rfind("ACK:" + kind, 0) == 0 && field(m, "name") == name) return m;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return "";
    };

    std::cout << "[PUB-RUNNER] RPI5(auto_sub) 연결 대기 (5초)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::vector<std::pair<Scenario, int>> results;  // (시나리오, 실제수신)

    for (const auto& s : scen)
    {
        std::cout << "\n================ [" << s.name << "] " << s.desc << " ================" << std::endl;
        std::string runMsg = "CMD:RUN|name=" + s.name + "|rel=" + s.sub_rel + "|dur=" + s.sub_dur +
                             "|his=" + s.his + "|depth=" + std::to_string(s.depth) +
                             "|part=" + s.sub_part + "|filter=" + s.filter +
                             "|mode=" + (s.late ? "late" : "normal");

        DataWriterQos wq = DATAWRITER_QOS_DEFAULT;
        wq.reliability().kind = relOf(s.pub_rel);
        wq.durability().kind = durOf(s.pub_dur);
        wq.history().kind = hisOf(s.his);
        if (s.depth > 0) wq.history().depth = s.depth;

        // Partition QoS (Publisher측): 매 시나리오 재설정 (없으면 기본 파티션)
        PublisherQos pqos;
        dpub->get_qos(pqos);
        pqos.partition().clear();
        if (!s.pub_part.empty()) pqos.partition().push_back(s.pub_part.c_str());
        dpub->set_qos(pqos);

        int count = -1;

        if (s.late)
        {
            // 늦은 조인: Pub 먼저 만들어 송신 → 그 다음 Sub가 조인
            DataWriter* dw = dpub->create_datawriter(dataTopic, wq);
            for (int i = 1; i <= s.samples; ++i)
            { HelloWorld m; m.index(i); m.message("Hello DDS " + std::to_string(i)); dw->write(&m);
              std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
            std::cout << "[PUB-RUNNER] (late) " << s.samples << "개 선송신 완료. Sub 조인 요청." << std::endl;
            sendCtrl(runMsg);
            if (waitAck("READY", s.name, 8).empty()) std::cout << "[PUB-RUNNER] READY 타임아웃" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));  // history 전달 시간
            sendCtrl("CMD:STOP|name=" + s.name);
            std::string res = waitAck("RES", s.name, 8);
            if (!res.empty()) count = std::atoi(field(res, "count").c_str());
            dpub->delete_datawriter(dw);
        }
        else
        {
            // 일반: Sub 먼저 Reader 준비 → 그 다음 Pub 송신
            sendCtrl(runMsg);
            if (waitAck("READY", s.name, 8).empty()) std::cout << "[PUB-RUNNER] READY 타임아웃" << std::endl;
            DataWriter* dw = dpub->create_datawriter(dataTopic, wq);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 매칭 시간
            for (int i = 1; i <= s.samples; ++i)
            { HelloWorld m; m.index(i); m.message("Hello DDS " + std::to_string(i)); dw->write(&m);
              std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            sendCtrl("CMD:STOP|name=" + s.name);
            std::string res = waitAck("RES", s.name, 8);
            if (!res.empty()) count = std::atoi(field(res, "count").c_str());
            dpub->delete_datawriter(dw);
        }

        std::cout << "[PUB-RUNNER] [" << s.name << "] 결과: 수신=" << count
                  << " (기대=" << s.expected << ")" << std::endl;
        results.push_back({s, count});
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    sendCtrl("CMD:QUIT");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // ---- 결과표 ----
    std::cout << "\n\n=================== 자동 시나리오 결과 요약 ===================" << std::endl;
    printf("%-10s %-34s %8s %8s %6s\n", "시나리오", "설명", "수신", "기대", "판정");
    int pass = 0;
    for (auto& pr : results)
    {
        const Scenario& s = pr.first; int c = pr.second;
        bool ok = false;
        if (c < 0) ok = false;
        else if (s.check == EXACT) ok = (c == s.expected);
        else if (s.check == ZERO) ok = (c == 0);
        else if (s.check == NONZERO) ok = (c > 0);
        if (ok) ++pass;
        printf("%-10s %-34s %8d %8d %6s\n", s.name.c_str(), s.desc.c_str(), c, s.expected, ok ? "PASS" : "FAIL");
    }
    std::cout << "-------------------------------------------------------------" << std::endl;
    std::cout << "PASS " << pass << " / " << results.size() << std::endl;

    factory->delete_participant(part);
    return 0;
}
