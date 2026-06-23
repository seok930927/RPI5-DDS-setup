// 시나리오 통합 Publisher — QoS/동작을 커맨드라인 옵션으로 설정
#include "HelloWorldPubSubTypes.hpp"
#include "qos_args.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <chrono>
#include <iostream>
#include <thread>

using namespace eprosima::fastdds::dds;

class PubListener : public DataWriterListener
{
public:
    int matched = 0;
    void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override
    {
        matched = info.current_count;
        std::cout << "[PUB] 매칭 변화: 현재 매칭된 Subscriber 수 = " << matched << std::endl;
    }
};

int main(int argc, char** argv)
{
    QosArgs args;
    if (!parse_args(argc, argv, args)) return 0;
    print_qos("PUB", args);

    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(args.domain, PARTICIPANT_QOS_DEFAULT);
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(participant);

    Topic* topic = participant->create_topic(args.topic, type.get_type_name(), TOPIC_QOS_DEFAULT);
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);

    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = args.reliability;
    wqos.durability().kind = args.durability;
    wqos.history().kind = args.history;
    wqos.history().depth = args.depth;

    PubListener listener;
    DataWriter* writer = publisher->create_datawriter(topic, wqos, &listener);
    if (!writer) { std::cerr << "[PUB] DataWriter 생성 실패 (QoS 확인)" << std::endl; return 1; }

    std::cout << "[PUB] 시작. (매칭 대기 최대 3초)" << std::endl;
    for (int i = 0; i < 30 && listener.matched == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    HelloWorld sample;
    for (int i = 1; i <= args.samples; ++i)
    {
        sample.index(static_cast<uint32_t>(i));
        sample.message("Hello DDS " + std::to_string(i));
        writer->write(&sample);
        std::cout << "[PUB] 송신: index=" << i << " msg=\"" << sample.message() << "\"" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(args.interval_ms));
    }
    std::cout << "[PUB] " << args.samples << "개 송신 완료. keep-alive " << args.keep_alive << "초 유지..." << std::endl;

    // 늦은 Subscriber(3-2/3-3)나 HEARTBEAT 관찰(2-2)을 위해 유지
    for (int i = 0; i < args.keep_alive * 10; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return 0;
}
