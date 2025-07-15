#ifndef __UT_ROBOT_SDK_Bridge_SUBSCRIBER_HPP__
#define __UT_ROBOT_SDK_Bridge_SUBSCRIBER_HPP__

#include "yunji/robot/dds_bridge/dds_bridge_factory.hpp"

namespace yunji
{
namespace robot
{
    
template<typename MSG>
class BridgeSubscriber
{
public:
    explicit BridgeSubscriber(const std::string& BridgeName, int64_t queuelen = 0) :
        mBridgeName(BridgeName), mQueueLen(queuelen)
        {}

    void InitBridge(const std::function<void(const void*)>& handler, int64_t queuelen = 0)
    {
        mHandler = handler;
        mQueueLen = queuelen;
        
        if (mHandler)
        {
            mBridgePtr = BridgeFactory::Instance()->CreateRecvBridge<MSG>(mBridgeName, mHandler, mQueueLen);
        }
        else
        {
            ;;
        }
    }

    void CloseBridge()
    {
        mBridgePtr.reset();     //释放当前智能指针对象
    }

    uint64_t GetLastDataAvailableTime() const
    {
        if (mBridgePtr)
        {
            return mBridgePtr->GetLastDataAvailableTime();
        }

        return -1;
    }

    const std::string& GetBridgeName() const
    {
        return mBridgeName;
    }

private:
    std::string mBridgeName;

    int64_t mQueueLen;

    std::function<void(const void*)> mHandler;

    BridgePtr<MSG> mBridgePtr;
};

template<typename MSG>
using BridgeSubscriberPtr = std::shared_ptr<BridgeSubscriber<MSG>>;

}
}

#endif//__UT_ROBOT_SDK_Bridge_SUBSCRIBER_HPP__
