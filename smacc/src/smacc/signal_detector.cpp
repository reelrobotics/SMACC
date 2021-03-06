/*****************************************************************************************************************
 * ReelRobotix Inc. - Software License Agreement      Copyright (c) 2018
 * 	 Authors: Pablo Inigo Blasco, Brett Aldrich
 *
 ******************************************************************************************************************/
#include <smacc/smacc_signal_detector.h>
#include <smacc/client_bases/smacc_action_client_base.h>
#include <smacc/smacc_state_machine.h>
#include <thread>

namespace smacc
{
/**
******************************************************************************************************************
* SignalDetector()
******************************************************************************************************************
*/
SignalDetector::SignalDetector(SmaccFifoScheduler *scheduler)
{
    scheduler_ = scheduler;
    loop_rate_hz = 10.0;
    end_ = false;
}

/**
******************************************************************************************************************
* initialize()
******************************************************************************************************************
*/
void SignalDetector::initialize(ISmaccStateMachine *stateMachine)
{
    smaccStateMachine_ = stateMachine;
    lastState_ = std::numeric_limits<unsigned long>::quiet_NaN();
    findUpdatableClients();
}

/**
******************************************************************************************************************
* findUpdatableClients()
******************************************************************************************************************
*/
void SignalDetector::findUpdatableClients()
{
    this->updatableClients_.clear();
    for (auto pair : this->smaccStateMachine_->getOrthogonals())
    {
        auto &orthogonal = pair.second;
        auto &clients = orthogonal->getClients();

        for (auto &client : clients)
        {
            // updatable client components
            auto updatableClient = dynamic_cast<ISmaccUpdatable *>(client.get());

            if (updatableClient != nullptr)
            {
                ROS_DEBUG_STREAM("Adding updatable client: " << demangleType(typeid(updatableClient)));
                this->updatableClients_.push_back(updatableClient);
            }

            // updatable client components
            std::vector<std::shared_ptr<ISmaccComponent>> components;
            client->getComponents(components);
            for (auto &componententry : components)
            {
                auto updatableComponent = dynamic_cast<ISmaccUpdatable *>(componententry.get());
                if (updatableComponent != nullptr)
                {
                    ROS_DEBUG_STREAM("Adding updatable component: " << demangleType(typeid(updatableComponent)));
                    this->updatableClients_.push_back(updatableComponent);
                }
            }
        }
    }
}

/**
******************************************************************************************************************
* findUpdatableClientBehaviors()
******************************************************************************************************************
*/
void SignalDetector::findUpdatableStateElements(ISmaccState *currentState)
{
    this->updatableStateElements_.clear();
    for (auto pair : this->smaccStateMachine_->getOrthogonals())
    {
        auto &orthogonal = pair.second;
        auto &behaviors = orthogonal->getClientBehaviors();

        for (auto &currentBehavior : behaviors)
        {
            ISmaccUpdatable *updatableClientBehavior = dynamic_cast<ISmaccUpdatable *>(currentBehavior.get());

            if (updatableClientBehavior != nullptr)
            {
                ROS_DEBUG_STREAM("Adding updatable behavior: " << demangleType(typeid(updatableClientBehavior)));
                this->updatableStateElements_.push_back(updatableClientBehavior);
            }
        }
    }

    auto updatableState = dynamic_cast<ISmaccUpdatable *>(currentState);
    if (updatableState != nullptr)
    {
        this->updatableStateElements_.push_back(updatableState);
    }

    auto statereactors = currentState->getStateReactors();
    for (auto &sr : statereactors)
    {
        ISmaccUpdatable *updatableStateReactor = dynamic_cast<ISmaccUpdatable *>(sr.get());
        if (updatableStateReactor != nullptr)
        {
            ROS_DEBUG_STREAM("Adding updatable stateReactorr: " << demangleType(typeid(updatableStateReactor)));
            this->updatableStateElements_.push_back(updatableStateReactor);
        }
    }
}

/**
******************************************************************************************************************
* setProcessorHandle()
******************************************************************************************************************
*/
void SignalDetector::setProcessorHandle(SmaccFifoScheduler::processor_handle processorHandle)
{
    processorHandle_ = processorHandle;
}

/**
******************************************************************************************************************
* runThread()
******************************************************************************************************************
*/
void SignalDetector::runThread()
{
    signalDetectorThread_ = boost::thread(boost::bind(&SignalDetector::pollingLoop, this));
}

/**
******************************************************************************************************************
* join()
******************************************************************************************************************
*/
void SignalDetector::join()
{
    signalDetectorThread_.join();
}

/**
******************************************************************************************************************
* stop()
******************************************************************************************************************
*/
void SignalDetector::stop()
{
    end_ = true;
}

/**
******************************************************************************************************************
* poll()
******************************************************************************************************************
*/
void SignalDetector::pollOnce()
{
    if (smaccStateMachine_ == nullptr)
    {
        // skip if the state machine is not properly initialized yet
        ROS_DEBUG("[PollOnce] update but state machine is not yet set.");
        return;
    }

    try
    {
        smaccStateMachine_->lockStateMachine("update behaviors");
        this->findUpdatableClients();
        ROS_DEBUG_STREAM("updatable clients: " << this->updatableClients_.size());

        if (this->updatableClients_.size())
        {
            for (auto *updatableClient : this->updatableClients_)
            {
                ROS_DEBUG_STREAM("[PollOnce] update client call:  " << demangleType(typeid(updatableClient)));
                updatableClient->executeUpdate();
            }
        }

        long currentStateIndex = smaccStateMachine_->getCurrentStateCounter();

        ROS_DEBUG_STREAM("[PollOnce] update behaviors. checking current state");

        auto currentState = smaccStateMachine_->getCurrentState();
        if (currentState != nullptr)
        {
            ROS_DEBUG_STREAM("[PollOnce] current state: " << currentStateIndex);
            ROS_DEBUG_STREAM("[PollOnce] last state: " << this->lastState_);

            if (currentStateIndex != 0)
            {
                if (currentStateIndex != this->lastState_)
                {
                    ROS_DEBUG_STREAM("[PollOnce] detected new state, refreshing updatable client behavior table");
                    // we are in a new state, refresh the updatable client behaviors table
                    this->lastState_ = currentStateIndex;
                    this->findUpdatableStateElements(currentState);
                }

                ROS_DEBUG_STREAM("updatable state elements: " << this->updatableStateElements_.size());
                for (auto *udpatableStateElement : this->updatableStateElements_)
                {
                    ROS_DEBUG_STREAM("pollOnce update client behavior call: " << demangleType(typeid(*udpatableStateElement)));
                    udpatableStateElement->executeUpdate();
                }
            }
        }
    }
    catch (...)
    {
        ROS_ERROR("Exception during Signal Detector update loop.");
    }
    smaccStateMachine_->unlockStateMachine("update behaviors");
}

/**
******************************************************************************************************************
* pollingLoop()
******************************************************************************************************************
*/
void SignalDetector::pollingLoop()
{
    ros::NodeHandle nh("~");


    if (!nh.getParam("signal_detector_loop_freq", this->loop_rate_hz))
    {
        ROS_WARN("Signal detector frequency (ros param ~signal_detector_loop_freq) was not set, using default frequency: %lf",  this->loop_rate_hz);
    }
    else
    {
        ROS_WARN("Signal detector frequency (ros param ~signal_detector_loop_freq): %lf",  this->loop_rate_hz);
    }

    nh.setParam("signal_detector_loop_freq", this->loop_rate_hz);

    ROS_INFO_STREAM("[SignalDetector] loop rate hz:" << loop_rate_hz);

    ros::Rate r(loop_rate_hz);
    while (ros::ok() && !end_)
    {
        ROS_INFO_STREAM_THROTTLE(10, "[SignalDetector] heartbeat");
        pollOnce();
        ros::spinOnce();
        r.sleep();
    }
}
}