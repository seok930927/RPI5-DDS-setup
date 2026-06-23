// 단독 테스트용 Subscriber
#include "HelloWorldPubSubTypes.hpp"

#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
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

int main()
{
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(0, PARTICIPANT_QOS_DEFAULT);
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(participant);

    Topic* topic = participant->create_topic("HelloWorldTopic", type.get_type_name(), TOPIC_QOS_DEFAULT);
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

    SubListener listener;
    DataReader* reader = subscriber->create_datareader(topic, DATAREADER_QOS_DEFAULT, &listener);

    std::cout << "[SUB] Subscriber 시작. 데이터 수신 대기..." << std::endl;

    // 20개 수신하거나 최대 15초까지 대기
    for (int i = 0; i < 150 && listener.received.load() < 20; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[SUB] 총 수신 개수 = " << listener.received.load() << std::endl;
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return (listener.received.load() > 0) ? 0 : 2;
}
