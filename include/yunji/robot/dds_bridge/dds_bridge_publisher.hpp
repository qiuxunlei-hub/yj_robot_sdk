#ifndef __YJ_ROBOT_SDK_CHANNEL_PUBLISHER_HPP__
#define __YJ_ROBOT_SDK_CHANNEL_PUBLISHER_HPP__

/**
 * @file bridge_publisher.hpp
 * @brief DDS消息发布者通道封装类
 * @copyright Copyright (c) 2025 YunJi Robotics. All rights reserved.
 */

#include <yunji/robot/bridge/bridge_factory.hpp>

namespace yunji
{

namespace robot
{

/**
 * @class BridgePublisher
 * @brief 泛型DDS消息发布者模板类
 * @tparam MSG 发布的消息类型（需支持DDS序列化）
 */
template<typename MSG>
class BridgePublisher
{
public:

    /**
     * @brief 显式构造函数
     * @param TopicName 通道名称（对应DDS Topic名称）
     */
    explicit BridgePublisher(const std::string& TopicName) :
        mTopicName(TopicName)
    {}

     /**
     * @brief 初始化Bridge
     * @note 需在调用Write()前执行
     */
    void IniteBridge()
    {
        mBridgePtr = ChannelFactory::Instance()->CreateSendChannel<MSG>(mTopicName);
    }

    /**
     * @brief 发布消息
     * @param msg 要发送的消息对象（常量引用）
     * @param waitMicrosec 写入超时时间（微秒，0表示非阻塞）
     * @return true 发送成功，false 发送失败或通道未初始化
     */
    bool Write(const MSG& msg, int64_t waitMicrosec = 0)
    {
        if (mBridgePtr)
        {
            return mBridgePtr->Write(msg, waitMicrosec);
        }

        return false;
    }

    /**
     * @brief 关闭通道（释放DDS资源）
     */
    void CloseBridge()
    {
        mBridgePtr.reset();
    }

    /**
     * @brief 获取BridgePtr Topic名称
     * @return const std::string& 通道名称引用
     */
    const std::string& GetChannelName() const
    {
        return mTopicName;
    }

private:
    std::string mTopicName;             //< DDS Topic 名称
    
    ChannelPtr<MSG> mBridgePtr;
};

template<typename MSG>
using ChannelPublisherPtr = std::shared_ptr<ChannelPublisher<MSG>>;

}
}

#endif//__UT_ROBOT_SDK_CHANNEL_PUBLISHER_HPP__
