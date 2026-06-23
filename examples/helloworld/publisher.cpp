// 단독 테스트용 Publisher
#include "HelloWorldPubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
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

int main()
{
    DomainParticipant* participant =
        DomainParticipantFactory::get_instance()->create_participant(0, PARTICIPANT_QOS_DEFAULT);
    TypeSupport type(new HelloWorldPubSubType());
    type.register_type(participant);

    Topic* topic = participant->create_topic("HelloWorldTopic", type.get_type_name(), TOPIC_QOS_DEFAULT);
    Publisher* publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT);

    PubListener listener;
    DataWriter* writer = publisher->create_datawriter(topic, DATAWRITER_QOS_DEFAULT, &listener);

    std::cout << "[PUB] Publisher 시작. Subscriber 매칭 대기..." << std::endl;
    // 매칭될 때까지 잠깐 대기 (최대 ~3초)
    for (int i = 0; i < 30 && listener.matched == 0; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    HelloWorld sample;
    for (uint32_t i = 1; i <= 20; ++i)
    {
        sample.index(i);
        sample.message("Hello DDS " + std::to_string(i));
        writer->write(&sample);
        std::cout << "[PUB] 송신: index=" << i << " msg=\"" << sample.message() << "\"" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    std::cout << "[PUB] 20개 송신 완료." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return 0;
}
