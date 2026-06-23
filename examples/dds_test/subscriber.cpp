// 시나리오 통합 Subscriber — QoS/동작을 커맨드라인 옵션으로 설정
#include "HelloWorldPubSubTypes.hpp"
#include "qos_args.hpp"

#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace eprosima::fastdds::dds;

class SubListener : public DataReaderListener
{
public:
    std::atomic<int> received{0};
    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override
    {
        std::cout << "[SUB] 매칭 변화: 현재 매칭된 Publisher 수 = " << info.current_count << std::endl;
    }
    void on_data_available(DataReader* reader) override
    {
        HelloWorld sample;
        SampleInfo info;
        while (reader->take_next_sample(&sample, &info) == RETCODE_OK)
        {
            if (info.valid_data)
            {
                int n = ++received;
                std::cout << "[SUB] 수신(" << n << "): index=" << sample.index()
                          << " msg=\"" << sample.message() << "\"" << std::endl;
            }
        }
    }
};

int main(int argc, char** argv)
{
    QosArgs args;
    if (!parse_args(argc, argv, args)) return 0;
    print_qos("SUB", args);

    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(args.domain, PARTICIPANT_QOS_DEFAULT);
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(participant);

    Topic* topic = participant->create_topic(args.topic, type.get_type_name(), TOPIC_QOS_DEFAULT);
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = args.reliability;
    rqos.durability().kind = args.durability;
    rqos.history().kind = args.history;
    rqos.history().depth = args.depth;

    SubListener listener;
    DataReader* reader = subscriber->create_datareader(topic, rqos, &listener);
    if (!reader) { std::cerr << "[SUB] DataReader 생성 실패 (QoS 확인)" << std::endl; return 1; }

    std::cout << "[SUB] 시작. 최대 " << args.timeout << "초 대기 (Ctrl+C로 종료 가능)" << std::endl;

    // timeout 동안 대기 (samples개 수신하면 조기 종료)
    int ticks = args.timeout * 10;
    for (int i = 0; i < ticks && listener.received.load() < args.samples; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "[SUB] 총 수신 개수 = " << listener.received.load() << std::endl;
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return (listener.received.load() > 0) ? 0 : 2;
}
