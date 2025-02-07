
#include "Generated/ObstaclesPubSubTypes.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
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
#include "blackboard.h"
#include <iostream> 
#include <fcntl.h>
#include <cjson/cJSON.h>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

newBlackboard *bb;
ConfigDDS * cd;
sem_t *sem;


void read_json(ConfigDDS * cd, bool first_time) {
    printf("Reading JSON\n");
    const char *filename = JSON_PATH;
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open file");
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)malloc(length + 1);
    if (!data) {
        perror("Memory allocation failed for JSON config");
        fclose(file);
    }
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);
    cJSON *json = cJSON_Parse(data);
    if (!json) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
    }    
    printf("parsed some json\n");
    cJSON *item;  // TODO CHECK FOR NULL and CORRUPTED JSON
    if (first_time) {
        if ((item = cJSON_GetObjectItem(json, "domainnum")) && cJSON_IsNumber(item))
            cd->domainnum = item->valueint;
        if ((item = cJSON_GetObjectItem(json, "discoveryport")) && cJSON_IsNumber(item))
            cd->discoveryport = item->valueint;
        if ((item = cJSON_GetObjectItem(json, "topicobstacles")) && cJSON_IsString(item))
            strncpy(cd->topicobstacles, item->valuestring, sizeof(cd->topicobstacles) - 1);
        if ((item = cJSON_GetObjectItem(json, "topictargets")) && cJSON_IsString(item))
            strncpy(cd->topictargets, item->valuestring, sizeof(cd->topictargets) - 1);
        if ((item = cJSON_GetObjectItem(json, "transmitterip")) && cJSON_IsString(item))
            strncpy(cd->transmitterip, item->valuestring, sizeof(cd->transmitterip) - 1);
        if ((item = cJSON_GetObjectItem(json, "receiverip")) && cJSON_IsString(item))
            strncpy(cd->receiverip, item->valuestring, sizeof(cd->receiverip) - 1);
    }

    cJSON_Delete(json);
    free(data);
}



void parseIP(const char* ip, int * octets) {
    std::stringstream test(ip);
    std::string segment;
    std::vector<std::string> seglist;
    while(std::getline(test, segment, '.')){
        seglist.push_back(segment);
    }
    for (int i = 0; i < seglist.size(); i++){
        int num = std::stoi(seglist[i]);
        octets[i] = num;
    }
}

class CustomIdlSubscriber
{
private:

    DomainParticipant* participant_;

    Subscriber* subscriber_;

    DataReader* reader_obstacle;
    DataReader* reader_target;

    Topic* topic_obstacle;
    Topic* topic_target;

    TypeSupport type_;

    class SubListenerObstacle : public DataReaderListener
    {
    public:

        SubListenerObstacle()
            : samples_(0)
        {
        }

        ~SubListenerObstacle() override
        {
        }

        void on_subscription_matched(
                DataReader*,
                const SubscriptionMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "Subscriber matched." << std::endl;
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

        void on_data_available(
                DataReader* reader) override
        {
            SampleInfo info;
            if (reader->take_next_sample(&my_obstacles, &info) == eprosima::fastdds::dds::RETCODE_OK)
            {
                std::cout << "Data available. Waiting for data..." << std::endl;
                if (info.valid_data)
                {
                    sem_wait(sem);
                    for (int i=0; i<MAX_OBJECTS; i++){ 
                        if (i < bb->n_obstacles){
                            bb->obstacle_xs[i] = my_obstacles.obstacles_x().at(i);
                            bb->obstacle_ys[i] = my_obstacles.obstacles_y().at(i);
                        } else {
                            bb->obstacle_xs[i] = -1;
                            bb->obstacle_ys[i] = -1;
                        }
                    }
                    sem_post(sem);
                }
            }
        }

        Obstacles my_obstacles;
        std::atomic_int samples_;
    } listener_obstacle;
    class SubListenerTarget : public DataReaderListener
    {
    public:
        SubListenerTarget()
            : samples_(0)
        {
        }

        ~SubListenerTarget() override
        {
        }

        void on_subscription_matched(
                DataReader*,
                const SubscriptionMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "Subscriber matched." << std::endl;
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

        void on_data_available(
                DataReader* reader) override
        {
            SampleInfo info;
            if (reader->take_next_sample(&my_targets, &info) == eprosima::fastdds::dds::RETCODE_OK)
            {
                std::cout << "Data Target available. Waiting for data..." << std::endl;
                if (info.valid_data)
                {
                    sem_wait(sem);
                    for (int i=0; i<MAX_OBJECTS; i++){ 
                        if (i < bb->n_targets){
                            bb->target_xs[i] = my_targets.obstacles_x().at(i);
                            bb->target_ys[i] = my_targets.obstacles_y().at(i);
                        } else {
                            bb->target_xs[i] = -1;
                            bb->target_ys[i] = -1;
                        }
                    }
                    sem_post(sem);
                }
            }
        }
        Obstacles my_targets;
        std::atomic_int samples_;
    } listener_target;

public:

    CustomIdlSubscriber()
        : participant_(nullptr)
        , subscriber_(nullptr)
        , topic_obstacle(nullptr)
        , reader_target(nullptr)
        , type_(new ObstaclesPubSubType())
    {
    }

    virtual ~CustomIdlSubscriber()
    {
        if (reader_obstacle != nullptr)
        {
            subscriber_->delete_datareader(reader_obstacle);
        }
        if (topic_obstacle != nullptr)
        {
            participant_->delete_topic(topic_obstacle);
        }
        if (reader_target != nullptr)
        {
            subscriber_->delete_datareader(reader_target);
        }
        if (topic_target != nullptr)
        {
            participant_->delete_topic(topic_target);
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    bool init(ConfigDDS *cd)
    {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");

        participantQos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = false;
        participantQos.wire_protocol().builtin.discovery_config.discoveryProtocol = DiscoveryProtocol::CLIENT;

        auto tcp_transport = std::make_shared<TCPv4TransportDescriptor>();
        tcp_transport->set_WAN_address(cd->transmitterip);
        tcp_transport->add_listener_port(8800);
        participantQos.transport().use_builtin_transports = false;
        participantQos.transport().user_transports.push_back(tcp_transport);

        int server_ip [4];
        parseIP(cd->receiverip, &server_ip[0]);
        Locator_t server_locator;
        server_locator.kind = LOCATOR_KIND_TCPv4;
        server_locator.port = cd->discoveryport;
        IPLocator::setIPv4(server_locator, server_ip[0], server_ip[1], server_ip[2], server_ip[3]);

        participantQos.wire_protocol().builtin.discovery_config.m_DiscoveryServers.push_back(server_locator);

        int client_ip [4];
        parseIP(cd->transmitterip, &client_ip[0]);
        Locator_t client_locator;
        client_locator.kind = LOCATOR_KIND_TCPv4;
        client_locator.port = 8800;
        IPLocator::setIPv4(client_locator, client_ip[0], client_ip[1], client_ip[2], client_ip[3]);

        participantQos.wire_protocol().builtin.metatrafficUnicastLocatorList.push_back(client_locator);

        participant_ = DomainParticipantFactory::get_instance()->create_participant(cd->domainnum, participantQos);

        if (participant_ == nullptr) {
            return false;
        }

        type_.register_type(participant_);

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        if (subscriber_ == nullptr)
        {
            return false;
        }
        topic_obstacle = participant_->create_topic("Topic1", type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic_obstacle == nullptr)
        {
            return false;
        }
        topic_target = participant_->create_topic("Topic2", type_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic_target == nullptr)
        {
            return false;
        }

        reader_obstacle = subscriber_->create_datareader(topic_obstacle, DATAREADER_QOS_DEFAULT, &listener_obstacle);
        if (reader_obstacle == nullptr)
        {
            return false;
        }
        reader_target = subscriber_->create_datareader(topic_target, DATAREADER_QOS_DEFAULT, &listener_target);
        if (reader_target == nullptr)
        {
            return false;
        }

        return true;
    }

    void run(int fd_obs, int fd_tar)
    {
        time_t now = time(NULL);
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (difftime(time(NULL), now) >= 0.5){
                send_heartbeat(fd_obs);
                send_heartbeat(fd_tar);
                now = time(NULL);
            }
        }
    }

};

int main()
{
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
    int fd_obs = open_watchdog_pipe(PIPE_OBSTACLE);
    int fd_tar = open_watchdog_pipe(PIPE_TARGET);

    cd = new ConfigDDS(); // Allocate memory for cd
    read_json(cd, true);
    printf("read ip hereeeeeeeeeee%s\n", cd->receiverip);

    logger("Obstacle and Target subscriber started. PID: %d", getpid());
    printf("receiverid %s\n", cd->receiverip);
    
    CustomIdlSubscriber* mysub = new CustomIdlSubscriber();
    if (mysub->init(cd)){
        mysub->run(fd_obs, fd_tar);
    }

    if (fd_obs >= 0) { close(fd_obs); }
    if (fd_tar >= 0) { close(fd_tar); }
    delete mysub;
    delete cd; // Free memory for cd
    return 0;
}