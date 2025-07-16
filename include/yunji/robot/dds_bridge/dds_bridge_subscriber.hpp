#ifndef __UT_ROBOT_SDK_Bridge_SUBSCRIBER_HPP__
#define __UT_ROBOT_SDK_Bridge_SUBSCRIBER_HPP__

#include "yunji/robot/dds_bridge/dds_bridge_factory.hpp"

namespace yunji
{
namespace robot
{
    

template <typename T>
class BridgeSubscriber {
public:
    using RawCallbackType = std::function<void(const T&)>;

    explicit BridgeSubscriber(const std::string& topic)
        : participant_(BridgeFactory::Instance()->GetParticipant()), topic_name_(topic) {}

    bool InitBridge(RawCallbackType callback, int queue_size = 1) {
        try {
            callback_ = callback;

            topic_ = std::make_shared<dds::topic::Topic<T>>(*participant_, topic_name_);
            subscriber_ = std::make_shared<dds::sub::Subscriber>(*participant_);
            reader_ = std::make_shared<dds::sub::DataReader<T>>(*subscriber_, *topic_);

            waitset_ = dds::core::cond::WaitSet();      //创建dds等待集

            cond_ = std::make_shared<dds::sub::cond::ReadCondition>(        //创建条件
                *reader_,
                dds::sub::status::DataState::any(),
                [this](dds::core::cond::Condition&) {
                    auto samples = reader_->take();
                    for (const auto& sample : samples) {
                        if (sample.info().valid()) {
                            callback_(sample.data());
                        }
                    }
                }
            );

            waitset_.attach_condition(*cond_);              //将条件附加到等待集

            wait_thread_ = std::thread([this]() {
                while (running_) {
                    try {
                        waitset_.dispatch(dds::core::Duration(2));
                    } catch (const dds::core::TimeoutError&) {
                        // 超时正常，忽略
                    } catch (const std::exception& e) {
                        std::cerr << "WaitSet dispatch error: " << e.what() << std::endl;
                    }
                }
            });

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Subscriber init failed: " << e.what() << std::endl;
            return false;
        }
    }

    ~BridgeSubscriber() {
        running_ = false;
        if (wait_thread_.joinable()) {
            wait_thread_.join();
        }
    }

private:
    std::shared_ptr<dds::domain::DomainParticipant> participant_;
    std::string topic_name_;
    std::shared_ptr<dds::topic::Topic<T>> topic_;
    std::shared_ptr<dds::sub::Subscriber> subscriber_;
    std::shared_ptr<dds::sub::DataReader<T>> reader_;

    std::shared_ptr<dds::sub::cond::ReadCondition> cond_;
    dds::core::cond::WaitSet waitset_;
    std::thread wait_thread_;
    std::atomic<bool> running_{true};

    RawCallbackType callback_;
};

}
}

#endif//__UT_ROBOT_SDK_Bridge_SUBSCRIBER_HPP__
