#include "Generated/ObstaclesPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include "blackboard.h"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

class ObstacleGenerator
{
private:
    DomainParticipant* participant_;
    Publisher* publisher_;
    Subscriber* subscriber_;
    Topic* topic_;
    DataWriter* writer_;
    DataReader* reader_;
    TypeSupport type_;
    bool receive_mode_;
    std::atomic<bool> data_available_;
    Obstacles received_obstacles_;
    
    class PubListener : public DataWriterListener
    {
    public:
        std::atomic_int matched_;
        PubListener() : matched_(0) {}
        void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override
        {
            matched_ += info.current_count_change;
            std::cout << (info.current_count_change == 1 ? "Publisher matched." : "Publisher unmatched.") << std::endl;
        }
    } pub_listener_;

    class SubListener : public DataReaderListener
    {
    private:
        ObstacleGenerator* parent_;
    public:
        SubListener(ObstacleGenerator* parent) : parent_(parent) {}
        void on_data_available(DataReader* reader) override
        {
            SampleInfo info;
            Obstacles obstacles;
            if (reader->take_next_sample(&obstacles, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
            {
                parent_->received_obstacles_ = obstacles;
                parent_->data_available_ = true;
                std::cout << "Obstacles received. Number: " << obstacles.obstacles_number << std::endl;
            }
        }
    } sub_listener_;

public:
    ObstacleGenerator(bool receive_mode)
        : participant_(nullptr), publisher_(nullptr), subscriber_(nullptr), topic_(nullptr),
          writer_(nullptr), reader_(nullptr), type_(new ObstaclesPubSubType()),
          receive_mode_(receive_mode), data_available_(false)
    {
        srand(static_cast<unsigned>(time(0)));
    }

    ~ObstacleGenerator()
    {
        if (writer_)
            publisher_->delete_datawriter(writer_);
        if (reader_)
            subscriber_->delete_datareader(reader_);
        if (publisher_)
            participant_->delete_publisher(publisher_);
        if (subscriber_)
            participant_->delete_subscriber(subscriber_);
        if (topic_)
            participant_->delete_topic(topic_);
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    bool init()
    {
        // Participant QoS with TCP
        DomainParticipantQos participant_qos;
        participant_qos.name("Obstacle_Generator_Participant");

        participant_qos.transport().use_builtin_transports = false;
        auto tcp_transport = std::make_shared<TCPv4TransportDescriptor>();
        tcp_transport->set_WAN_address("127.0.0.1"); 
        tcp_transport->add_listener_port(5100);
        participant_qos.transport().user_transports.push_back(tcp_transport);

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);
        if (!participant_)
            return false;

        // Register Type
        type_.register_type(participant_);

        // Create Topic
        topic_ = participant_->create_topic("ObstaclesTopic", type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (!topic_)
            return false;

        // Create Publisher and Subscriber
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        if (!publisher_ || !subscriber_)
            return false;

        // Create DataWriter
        if (!receive_mode_)
        {
            writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &pub_listener_);
            if (!writer_)
                return false;
        }

        // Create DataReader
        if (receive_mode_)
        {
            reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &sub_listener_);
            if (!reader_)
                return false;
        }

        return true;
    }

    void run()
    {
        if (receive_mode_)
        {
            std::cout << "Running in RECEIVE mode." << std::endl;
            while (true)
            {
                if (data_available_)
                {
                    data_available_ = false;
                    // Simulate storing received data in shared memory

                    std::cout << "Obstacles stored in shared memory." << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        else
        {
            std::cout << "Running in GENERATE & SEND mode." << std::endl;
            while (true)
            {
                Obstacles obstacles;
                generate_obstacles(obstacles);
                if (pub_listener_.matched_ > 0)
                {
                    writer_->write(&obstacles);
                    std::cout << "Obstacles published. Number: " << obstacles.obstacles_number << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

private:
    void generate_obstacles(Obstacles& obstacles)
    {
        obstacles.obstacles_number = rand() % 10 + 1;
        obstacles.obstacles_x.resize(obstacles.obstacles_number);
        obstacles.obstacles_y.resize(obstacles.obstacles_number);
        for (int i = 0; i < obstacles.obstacles_number; ++i)
        {
            obstacles.obstacles_x[i] = rand() % 100;
            obstacles.obstacles_y[i] = rand() % 100;
        }
    }
};

int main()
{
    // Set `receive_mode` to true for receiving, false for generating & sending
    bool receive_mode = true; // Adjust based on the mode you need

    ObstacleGenerator generator(receive_mode);
    if (generator.init())
    {
        generator.run();
    }
    else
    {
        std::cerr << "Failed to initialize Obstacle Generator." << std::endl;
    }

    return 0;
}
