
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
newBlackboard *bb;
sem_t *sem;

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
                    for (int i=0; i<MAX_OBSTACLES; i++){ // TODO MIX THESE TWO FORs
                        bb->obstacle_xs[i] = -1;
                        bb->obstacle_ys[i] = -1;
                    }
                    for (int i=0; i<bb->n_obstacles; i++){
                        bb->obstacle_xs[i] = my_obstacles.obstacles_x().at(i);
                        bb->obstacle_ys[i] = my_obstacles.obstacles_y().at(i);
                    }
                    sem_post(sem);
                    for (int i=0; i<my_obstacles.obstacles_number(); i++){ // TODO DELETE THIS LATER
                        std::cout << "Obstacle received " << i << " x: " << my_obstacles.obstacles_x().at(i) << " y: " << my_obstacles.obstacles_y().at(i) << std::endl;
                    }
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
                    for (int i=0; i<MAX_TARGETS; i++){ // TODO MIX THESE TWO FORs
                        bb->target_xs[i] = -1;
                        bb->target_ys[i] = -1;
                    }
                    for (int i=0; i<bb->n_targets; i++){
                        bb->target_xs[i] = my_targets.obstacles_x().at(i);
                        bb->target_ys[i] = my_targets.obstacles_y().at(i);
                    }
                    sem_post(sem);
                    for (int i=0; i<my_targets.obstacles_number(); i++){ // TODO DELETE THIS LATER
                        std::cout << "Target received " << i << " x: " << my_targets.obstacles_x().at(i) << " y: " << my_targets.obstacles_y().at(i) << std::endl;
                    }
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

    bool init()
    {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr)
        {
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

    void run()
    {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    std::cout << "Starting subscriber." << std::endl;

    CustomIdlSubscriber* mysub = new CustomIdlSubscriber();
    if (mysub->init())
    {
        mysub->run();
    }

    delete mysub;
    return 0;
}