
#include "Generated/ObstaclesPubSubTypes.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "blackboard.h"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
pthread_t obstacle_thread, target_thread;

class CustomIdlPublisher
{
private:

    Obstacles my_obstacles;
    Obstacles my_targets;

    DomainParticipant* participant_;

    Publisher* publisher_;

    Topic* topic_obstacle;
    Topic* topic_target;

    DataWriter* writer_obstacle;
    DataWriter* writer_target;

    TypeSupport type_;

    class PubListenerObstacle : public DataWriterListener
    {
    public:

        PubListenerObstacle()
            : matched_(0)
        {
        }

        ~PubListenerObstacle() override
        {
        }

        void on_publication_matched(
                DataWriter*,
                const PublicationMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher Obstacle matched." << std::endl;
            }
            else if (info.current_count_change == -1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                        << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
            }
        }

        std::atomic_int matched_;

    } listener_obstacle;
    class PubListenerTarget : public DataWriterListener
    {
    public:

        PubListenerTarget()
            : matched_(0)
        {
        }

        ~PubListenerTarget() override
        {
        }

        void on_publication_matched(
                DataWriter*,
                const PublicationMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher Target matched." << std::endl;
            }
            else if (info.current_count_change == -1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                        << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
            }
        }

        std::atomic_int matched_;

    } listener_target;

public:

    CustomIdlPublisher()
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_obstacle(nullptr)
        , topic_target(nullptr)
        , writer_obstacle(nullptr)        
        , writer_target(nullptr)
        , type_(new ObstaclesPubSubType())
    {
    }

    virtual ~CustomIdlPublisher()
    {
        if (writer_obstacle != nullptr)
        {
            publisher_->delete_datawriter(writer_obstacle);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        if (writer_target != nullptr)
        {
            publisher_->delete_datawriter(writer_target);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        if (topic_obstacle != nullptr)
        {
            participant_->delete_topic(topic_obstacle);
        }
        if (topic_target != nullptr)
        {
            participant_->delete_topic(topic_target);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    bool init()
    {   
        DomainParticipantQos participantQos;
        participantQos.name("Participant_publisher");

        participantQos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = false;
        participantQos.wire_protocol().builtin.discovery_config.discoveryProtocol = DiscoveryProtocol::SERVER;

        auto tcp_transport = std::make_shared<TCPv4TransportDescriptor>();
        tcp_transport->set_WAN_address("127.0.0.1");  
        tcp_transport->add_listener_port(8888);  // Listening port for discovery
        participantQos.transport().use_builtin_transports = false;
        participantQos.transport().user_transports.push_back(tcp_transport);

        Locator_t locator;
        locator.kind = LOCATOR_KIND_TCPv4;  
        locator.port = 8888;  // Server listens here
        IPLocator::setIPv4(locator, 127, 0, 0, 1);

        participantQos.wire_protocol().builtin.metatrafficUnicastLocatorList.push_back(locator);

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);
        if (participant_ == nullptr)
        {
            std::cerr << "Failed to create DomainParticipant" << std::endl;
            return false;
        }
        type_.register_type(participant_);
        
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
        if (publisher_ == nullptr)
        {
            return false;
        }

        // Obstacles
        topic_obstacle = participant_->create_topic("Topic1", type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic_obstacle == nullptr)
        {
            return false;
        }
        writer_obstacle = publisher_->create_datawriter(topic_obstacle, DATAWRITER_QOS_DEFAULT, &listener_obstacle);
        if (writer_obstacle == nullptr)
        {
            return false;
        }
        // Targets
        topic_target = participant_->create_topic("Topic2", type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic_target == nullptr)
        {
            return false;
        }
        writer_target = publisher_->create_datawriter(topic_target, DATAWRITER_QOS_DEFAULT, &listener_target);

        if (writer_target == nullptr)
        {
            return false;
        }
        return true;
    }   

    int publish(int * obj_x, int * obj_y, DataWriter* writer, Obstacles my_pubsublist)
    {
        for (int i=0; i<MAX_OBJECTS; i++){ 
            int gen_x = rand() % (WIN_SIZE_X-1);
            int gen_y = rand() % (WIN_SIZE_Y-1);
            while (gen_x <= 0 && gen_y <= 0){
                gen_x = rand() % (WIN_SIZE_X-1);
                gen_y = rand() % (WIN_SIZE_Y-1);
            }
            obj_x[i] = gen_x;
            obj_y[i] = gen_y;
        }
        std::vector<int32_t> obstacles_x(obj_x, obj_x + MAX_OBJECTS);
        std::vector<int32_t> obstacles_y(obj_y, obj_y + MAX_OBJECTS);
        my_pubsublist.obstacles_number(MAX_OBJECTS);
        my_pubsublist.obstacles_x(obstacles_x);
        my_pubsublist.obstacles_y(obstacles_y);
        writer->write(&my_pubsublist);
        return my_pubsublist.obstacles_number();
    }

    void run()
    {
        int obs_x[MAX_OBJECTS], obs_y[MAX_OBJECTS];
        int tar_x[MAX_OBJECTS], tar_y[MAX_OBJECTS];
        int written;
        srand((unsigned int)time(NULL));
        while (true)
        {
            if (listener_obstacle.matched_>0){  // obstacle topic
                written = publish(obs_x, obs_y, writer_obstacle, my_obstacles);
                std::cout << "Message: " << written <<"targets generated and SENT!" << std::endl;
            }
            if (listener_target.matched_>0){    // target topic
                written = publish(tar_x, tar_y, writer_target, my_targets);
                std::cout << "Message: " << written <<"targets generated and SENT!" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(3000)); //TODO IMPLEMENT TWO THREADS THAT RUN FOR TARGETS AND OBSTACLES
        }
    }
};

int main()
{
    std::cout << "Starting publisher." << std::endl;

    CustomIdlPublisher* mypub = new CustomIdlPublisher();
    if(mypub->init())
    {
        mypub->run();
    }

    delete mypub;
    return 0;
}