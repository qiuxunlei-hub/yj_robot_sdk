#ifndef __YJ_ROBOT_SDK_Bridge_PUBLISHER_HPP__
#define __YJ_ROBOT_SDK_Bridge_PUBLISHER_HPP__

/**
 * @file bridge_publisher.hpp
 * @brief DDS消息发布者通道封装类
 * @copyright Copyright (c) 2025 YunJi Robotics. All rights reserved.
 */

#include "yunji/robot/dds_bridge/dds_bridge_factory.hpp"

namespace yunji
{

namespace robot
{

template <typename T> class BridgePublisher;

template <typename T>
using BridgePublisherPtr = std::unique_ptr<BridgePublisher<T>>;


/**
 * @class BridgePublisher
 * @brief 泛型DDS消息发布者模板类
 * @tparam MSG 发布的消息类型（需支持DDS序列化）
 */
template <typename T>
class BridgePublisher {
public:
    explicit BridgePublisher(const std::string& topic)
        : participant_(BridgeFactory::Instance()->GetParticipant()), topic_name_(topic) {}

    bool InitBridge() {
        try {
            topic_ = std::make_shared<dds::topic::Topic<T>>(*participant_, topic_name_);
            publisher_ = std::make_shared<dds::pub::Publisher>(*participant_);
            writer_ = std::make_shared<dds::pub::DataWriter<T>>(* publisher_, *topic_);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Publisher init failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool Write(const T& msg) {
        try {
            writer_->write(msg);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Publish error: " << e.what() << std::endl;
            return false;
        }
    }

private:
    std::shared_ptr<dds::domain::DomainParticipant> participant_;
    std::string topic_name_;
    std::shared_ptr<dds::topic::Topic<T>> topic_;
    std::shared_ptr<dds::pub::Publisher> publisher_;
    std::shared_ptr<dds::pub::DataWriter<T>> writer_;
};

}
}

#endif//__UT_ROBOT_SDK_Bridge_PUBLISHER_HPP__
