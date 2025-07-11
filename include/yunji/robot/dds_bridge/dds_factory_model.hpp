#ifndef __YUNJI_COMMON_DDS_DDS_FACTORY_MODEL_HPP__
#define __YUNJI_COMMON_DDS_DDS_FACTORY_MODEL_HPP__

/**
 * @file dds_factory_model.hpp
 * @brief DDS抽象层核心实现（基于CycloneDDS）
 * @note 提供与具体DDS实现无关的通用接口
 */

#include <dds/dds.hpp>  // CycloneDDS核心头文件
#include <memory>
#include <string>
#include <functional>
#include <mutex>

namespace yunji {
namespace common {

/**
 * @class DdsTopicChannel
 * @brief DDS主题通道模板类（类型安全封装）
 * @tparam T 消息类型
 */
template<typename T>
class DdsTopicChannel {
public:
    /**
     * @brief 写入消息
     * @param msg 消息对象
     * @param waitMicrosec 等待超时时间（微秒）
     * @return true 写入成功，false 写入失败
     */
    virtual bool Write(const T& msg, int64_t waitMicrosec = 0) = 0;
    
    virtual ~DdsTopicChannel() = default;
};

/**
 * @brief DDS主题通道智能指针模板
 * @tparam T 消息类型
 */
template<typename T>
using DdsTopicChannelPtr = std::shared_ptr<DdsTopicChannel<T>>;

/**
 * @class DdsFactoryModel
 * @brief DDS工厂模型（抽象层核心）
 * 
 * 提供与具体DDS实现无关的接口，封装以下功能：
 * 1. DDS域初始化
 * 2. 主题通道创建
 * 3. 读写器设置
 */
class DdsFactoryModel : public std::enable_shared_from_this<DdsFactoryModel> {
public:
    using Ptr = std::shared_ptr<DdsFactoryModel>;
    
    DdsFactoryModel();
    virtual ~DdsFactoryModel();
    
    /**
     * @brief 初始化DDS域
     * @param domainId 域ID
     * @param networkInterface 网络接口
     */
    void Init(int32_t domainId, const std::string& networkInterface = "");
    
    /**
     * @brief 通过配置文件初始化
     * @param configFileName 配置文件路径
     */
    void Init(const std::string& configFileName);
    
    /**
     * @brief 释放所有DDS资源
     */
    void Release();
    
    /**
     * @brief 创建主题通道
     * @tparam T 消息类型
     * @param topicName 主题名称
     * @return DdsTopicChannelPtr<T> 主题通道智能指针
     */
    template<typename T>
    DdsTopicChannelPtr<T> CreateTopicChannel(const std::string& topicName) {
        auto channel = std::make_shared<DdsTopicChannelImpl<T>>(shared_from_this(), topicName);
        return channel;
    }
    
    /**
     * @brief 设置写入器
     * @tparam T 消息类型
     * @param channel 主题通道
     */
    template<typename T>
    void SetWriter(DdsTopicChannelPtr<T> channel) {
        auto impl = std::dynamic_pointer_cast<DdsTopicChannelImpl<T>>(channel);
        if (impl) {
            impl->CreateWriter();
        }
    }
    
    /**
     * @brief 设置读取器
     * @tparam T 消息类型
     * @param channel 主题通道
     * @param callback 数据回调函数
     * @param queueLen 队列长度
     */
    template<typename T>
    void SetReader(DdsTopicChannelPtr<T> channel, 
                  std::function<void(const void*)> callback, 
                  int32_t queueLen = 0) {
        auto impl = std::dynamic_pointer_cast<DdsTopicChannelImpl<T>>(channel);
        if (impl) {
            impl->CreateReader(callback, queueLen);
        }
    }
    
private:
    // 内部实现类
    template<typename T>
    class DdsTopicChannelImpl : public DdsTopicChannel<T> {
    public:
        DdsTopicChannelImpl(std::shared_ptr<DdsFactoryModel> factory, 
                           const std::string& topicName)
            : mFactory(factory), mTopicName(topicName) {}
        
        bool Write(const T& msg, int64_t waitMicrosec = 0) override {
            if (!mWriter) {
                return false;
            }
            try {
                mWriter->write(msg);
                return true;
            } catch (const dds::core::Exception& e) {
                // 错误处理
                return false;
            }
        }
        
        void CreateWriter() {
            if (!mWriter) {
                // 创建数据写入器
                dds::pub::qos::DataWriterQos qos;
                qos << dds::core::policy::Reliability::Reliable();
                mWriter = std::make_unique<dds::pub::DataWriter<T>>(
                    mFactory->mPublisher, mFactory->mTopicMap.at(mTopicName), qos);
            }
        }
        
        void CreateReader(std::function<void(const void*)> callback, int32_t queueLen) {
            if (!mReader) {
                // 创建数据读取器
                dds::sub::qos::DataReaderQos qos;
                qos << dds::core::policy::Reliability::Reliable();
                mReader = std::make_unique<dds::sub::DataReader<T>>(
                    mFactory->mSubscriber, mFactory->mTopicMap.at(mTopicName), qos);
                
                // 设置监听器
                mReader->listener([callback](dds::sub::DataReader<T>& reader) {
                    dds::sub::LoanedSamples<T> samples = reader.take();
                    for (const auto& sample : samples) {
                        if (sample.info().valid()) {
                            callback(static_cast<const void*>(&sample.data()));
                        }
                    }
                }, dds::core::status::StatusMask::data_available());
            }
        }
        
    private:
        std::shared_ptr<DdsFactoryModel> mFactory;
        std::string mTopicName;
        std::unique_ptr<dds::pub::DataWriter<T>> mWriter;
        std::unique_ptr<dds::sub::DataReader<T>> mReader;
    };
    
    // 获取或创建主题
    template<typename T>
    dds::topic::Topic<T> GetOrCreateTopic(const std::string& topicName) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mTopicMap.find(topicName);
        if (it != mTopicMap.end()) {
            return dds::topic::topic_cast<dds::topic::Topic<T>>(it->second);
        }
        
        dds::topic::Topic<T> topic(mParticipant, topicName);
        mTopicMap[topicName] = topic;
        return topic;
    }
    
private:
    dds::domain::DomainParticipant mParticipant;
    dds::pub::Publisher mPublisher;
    dds::sub::Subscriber mSubscriber;
    
    std::unordered_map<std::string, dds::topic::TopicDescription> mTopicMap;
    std::mutex mMutex;
    
    bool mInitialized = false;
};

using DdsFactoryModelPtr = DdsFactoryModel::Ptr;

} // namespace common
} // namespace unitree

#endif // __YUNJI_COMMON_DDS_DDS_FACTORY_MODEL_HPP__