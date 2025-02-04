#include "Generated/ObstaclesPubSubTypes.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.hpp>

// #include <fastdds/dds/core/ddscore.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
// #include <fastdds/dds/topic/DataWriter.hpp>


#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include "blackboard.h"

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
newBlackboard *bb;
sem_t *sem;

class CustomTransport
{
private:

    Obstacles my_message_;
    DomainParticipant* participant_;
    Publisher* publisher_;
    DataWriter* writer_;
    Subscriber *subscriber_;
    DataReader *reader_;
    Topic* topic_;
    TypeSupport type_;
    int send_or_not;

    class PubListener : public DataWriterListener
    {
    public:

        PubListener()
            : matched_(0)
        {
        }

        ~PubListener() override
        {
        }

        void on_publication_matched(
                DataWriter* writer,
                const PublicationMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher matched." << std::endl;
                eprosima::fastdds::rtps::LocatorList locators;
                writer->get_sending_locators(locators);
                for (const eprosima::fastdds::rtps::Locator& locator : locators)
                {
                    print_transport_protocol(locator);
                }

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
        private:
        void print_transport_protocol(const eprosima::fastdds::rtps::Locator &locator)
        {
            switch (locator.kind)
            {
            case LOCATOR_KIND_UDPv4:
                std::cout << "Using UDPv4" << std::endl;
                break;
            case LOCATOR_KIND_UDPv6:
                std::cout << "Using UDPv6" << std::endl;
                break;
            case LOCATOR_KIND_SHM:
                std::cout << "Using Shared Memory" << std::endl;
                break;
            default:
                std::cout << "Unknown Transport" << std::endl;
                break;
            }
        }

    } publistener_;

    class SubListener : public DataReaderListener
    {
    public:
        SubListener()
            : samples_(0)
        {
        }

        ~SubListener() override
        {
        }

        void on_subscription_matched(DataReader *reader, const SubscriptionMatchedStatus &info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "Subscriber matched." << std::endl;
                eprosima::fastdds::rtps::LocatorList locators;
                reader->get_listening_locators(locators);
                for (const eprosima::fastdds::rtps::Locator &locator : locators)
                {
                    print_transport_protocol(locator);
                }
            }
            else if (info.current_count_change == -1)
            {
                std::cout << "Subscriber unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                          << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
            }
        }

        void on_data_available(DataReader *reader) override
        {
            SampleInfo info;
            std::cout << "Data available. Waiting for data..." << std::endl;
            if (reader->take_next_sample(&my_message_, &info) == eprosima::fastdds::dds::RETCODE_OK)
            {
                std::cout << "Message received. Obstacles: " << my_message_.obstacles_number() << std::endl;
                if (info.valid_data)
                {
                    sem_wait(sem);
                    bb->n_obstacles = my_message_.obstacles_number();
                    for (int i=0; i<bb->n_obstacles; i++){
                        std::cout << "Obstacle " << i << " x: " << my_message_.obstacles_x().at(i) << " y: " << my_message_.obstacles_y().at(i) << std::endl;
                        bb->obstacle_xs[i] = my_message_.obstacles_x().at(i);
                        bb->obstacle_ys[i] = my_message_.obstacles_y().at(i);
                    }
                    sem_post(sem);
                    std::cout << "Obstacles received. Number: " << my_message_.obstacles_number() << std::endl;
                }
            }
        }

    public:
        Obstacles my_message_;
        std::atomic_int samples_;

    private:
        void print_transport_protocol(const eprosima::fastdds::rtps::Locator &locator)
        {
            switch (locator.kind)
            {
            case LOCATOR_KIND_UDPv4:
                std::cout << "Using UDPv4" << std::endl;
                break;
            case LOCATOR_KIND_UDPv6:
                std::cout << "Using UDPv6" << std::endl;
                break;
            case LOCATOR_KIND_SHM:
                std::cout << "Using Shared Memory" << std::endl;
                break;
            default:
                std::cout << "Unknown Transport" << std::endl;
                break;
            }
        }

    } sublistener_;
public:

    CustomTransport()
        : participant_(nullptr)
        , publisher_(nullptr)
        , writer_(nullptr)
        , subscriber_(nullptr)
        , reader_(nullptr)
        , topic_(nullptr)
        , type_(new ObstaclesPubSubType())
    {
    }

    virtual ~CustomTransport()
    {
        if (writer_ != nullptr)
        {
            publisher_->delete_datawriter(writer_);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);

        if (reader_ != nullptr)
        {
            subscriber_->delete_datareader(reader_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    bool init(int send_or_not)
    {
        DomainParticipantQos participantQos;
        
        participantQos.name("Participant");

        // participantQos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = false;
        // if (send_or_not == 1) {
        //     participantQos.wire_protocol().builtin.discovery_config.discoveryProtocol = DiscoveryProtocol::SERVER;
        //     participantQos.wire_protocol().builtin.discovery_config.m_DiscoveryServers.push_back(Locator_t());
        // }
        // else {
        //     participantQos.wire_protocol().builtin.discovery_config.discoveryProtocol = DiscoveryProtocol::CLIENT;
        //     Locator_t locator;
        //     locator.port = 8888;
        //     IPLocator::setIPv4(locator, 127,0,0,1);
        //     participantQos.wire_protocol().builtin.discovery_config.m_DiscoveryServers.push_back(locator);
        // }
        // participantQos.transport().use_builtin_transports = false;
        // auto tcp_transport = std::make_shared<TCPv4TransportDescriptor>();
        // tcp_transport->set_WAN_address("127.0.0.1"); 
        // tcp_transport->add_listener_port(8888);
        // participantQos.transport().user_transports.push_back(tcp_transport);

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);
        if (participant_ == nullptr)
        {
            return false;
        }
        type_.register_type(participant_);
        topic_ = participant_->create_topic("Topic1", type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic_ == nullptr)
        {
            return false;
        }
        if (send_or_not == 1) {
            publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
            if (publisher_ == nullptr)
            {
                return false;
            }
            writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &publistener_);
            if (writer_ == nullptr)
            {
                return false;
            }
        }
        else {
            subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
            if (subscriber_ == nullptr)
            {
                return false;
            }
            reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &sublistener_);
            if (reader_ == nullptr)
            {
                return false;
            }
        }
        return true;
    }

    bool publish(newBlackboard *bb, sem_t *sem)
    {
        if (publistener_.matched_ > 0)
        {
            int gen_x, gen_y;
            sem_wait(sem);
            for (int i=0; i<99; i++){
                bb->obstacle_xs[i] = -1;
                bb->obstacle_ys[i] = -1;
            }
            for (int i=0; i<bb->n_obstacles; i++){ 
                gen_x = rand() % (WIN_SIZE_X-1);
                gen_y = rand() % (WIN_SIZE_Y-1);
                while (gen_x == bb->drone_x && gen_y == bb->drone_y){
                    gen_x = rand() % (WIN_SIZE_X-1);
                    gen_y = rand() % (WIN_SIZE_Y-1);
                }
                bb->obstacle_xs[i] = gen_x;
                bb->obstacle_ys[i] = gen_y;
            }
            sem_post(sem);
            // sleep(4);
            std::vector<int32_t> obstacles_x(bb->obstacle_xs, bb->obstacle_xs + bb->n_obstacles);
            std::vector<int32_t> obstacles_y(bb->obstacle_ys, bb->obstacle_ys + bb->n_obstacles);
            my_message_.obstacles_number(bb->n_obstacles);
            my_message_.obstacles_x(obstacles_x);
            my_message_.obstacles_y(obstacles_y);
            writer_->write(&my_message_);
            return true;
        }
        return false;
    }

    void run(newBlackboard *bb, sem_t *sem, int send_or_not)
    {
        if (send_or_not == 1) {
            std::cout << "Running in GENERATE & SEND mode." << std::endl;
            while (true)
            {
                if (publish(bb, sem))
                {
                    std::cout << "Message: " << my_message_.obstacles_number() <<"obstacles generated and SENT!" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(7000)); 
            }
        }
        else {
            std::cout << "Running in RECEIVE mode." << std::endl;
            while (true) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
            }
        }
        
    }
};

int main()
{
    std::cout << "Starting Obstacle Generator..." << std::endl;

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return 1;
    }
    if (ftruncate(shm_fd, sizeof(newBlackboard)) == -1) {
        perror("ftruncate failed");
        return 1;
    }
    bb = static_cast<newBlackboard*>(
        mmap(NULL, sizeof(newBlackboard), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)
    );
    if (bb == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    memset(bb, 0, sizeof(newBlackboard));
    sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }
    srand((unsigned int)time(NULL));

    int send_or_not = 0;
    CustomTransport* mypub = new CustomTransport();
    if(mypub->init(send_or_not))
    {
        mypub->run(bb, sem, send_or_not);
    }
    delete mypub;
    return 0;
}
