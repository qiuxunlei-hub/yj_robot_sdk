#ifndef YUNJI_COMMON_DDS_DDS_FACTORY_MODEL_HPP
#define YUNJI_COMMON_DDS_DDS_FACTORY_MODEL_HPP

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
#include <unordered_map>
#include <typeindex>

namespace yunji {
namespace robot {

/**
 * @class DdsTopicChannel
 * @brief DDS主题通道抽象接口
 * @tparam MsgType 消息数据类型
 */
template<typename MsgType>
class DdsTopicChannel {
public:
    virtual ~DdsTopicChannel() = default;
    
    /**
     * @brief 写入消息到DDS网络
     * @param message 消息对象
     * @param timeout_us 等待超时时间（微秒，0表示非阻塞）
     * @return 是否写入成功
     */
    virtual bool Write(const MsgType& message) = 0;

    virtual uint64_t GetLastDataAvailableTime() = 0;
};

/// DDS主题通道智能指针
template<typename MsgType>
using DdsTopicChannelPtr = std::shared_ptr<DdsTopicChannel<MsgType>>;

/**
 * @class DdsFactory
 * @brief DDS工厂核心类
 * 
 * 封装DDS核心功能：
 * 1. DDS域初始化
 * 2. 主题通道管理
 * 3. 读写器生命周期管理
 * 
 * @note 线程安全设计
 */
class DdsFactory : public std::enable_shared_from_this<DdsFactory> {
public:
    using Ptr = std::shared_ptr<DdsFactory>;
    
    DdsFactory();
    virtual ~DdsFactory();
    
    // 禁用拷贝和赋值
    DdsFactory(const DdsFactory&) = delete;
    DdsFactory& operator=(const DdsFactory&) = delete;
    
    /**
     * @brief 初始化DDS域
     * @param domain_id DDS域ID
     * @param network_interface 绑定的网络接口(空字符串表示默认)
     * @throws std::runtime_error 初始化失败时抛出
     */
    void initialize(int32_t domain_id, const std::string& network_interface = "");
    
    /**
     * @brief 通过配置文件初始化
     * @param config_path 配置文件路径
     * @throws std::runtime_error 初始化失败时抛出
     */
    void initialize(const std::string& config_path);
    
    /**
     * @brief 释放所有DDS资源
     * @note 自动调用，也可手动提前释放
     */
    void shutdown();
    
    /**
     * @brief 创建主题通道
     * @tparam MsgType 消息类型
     * @param topic_name 主题名称
     * @return 主题通道智能指针
     */
    template<typename MsgType>
    DdsTopicChannelPtr<MsgType> create_topic_channel(const std::string& topic_name);
    
    /**
     * @brief 启用主题的写入功能
     * @tparam MsgType 消息类型
     * @param channel 主题通道
     */
    template<typename MsgType>
    void enable_writer(DdsTopicChannelPtr<MsgType> channel);
    
    /**
     * @brief 启用主题的读取功能
     * @tparam MsgType 消息类型
     * @param channel 主题通道
     * @param data_callback 数据到达回调函数
     * @param queue_capacity 内部队列容量(0表示默认)
     */
    template<typename MsgType>
    void enable_reader(DdsTopicChannelPtr<MsgType> channel, 
                      std::function<void(const MsgType&)> data_callback,
                      size_t queue_capacity = 0);

private:
    // 内部实现类
    template<typename MsgType>
    class TopicChannelImpl;
    
    // 获取或创建主题
    template<typename MsgType>
    dds::topic::Topic<MsgType> get_or_create_topic(const std::string& topic_name);
    
    // DDS核心实体
    dds::domain::DomainParticipant m_participant;
    dds::pub::Publisher m_publisher;
    dds::sub::Subscriber m_subscriber;
    
    // 主题管理
    std::mutex m_topic_mutex;    //互斥锁，用于保护topic资源

    std::unordered_map<std::string, dds::topic::TopicDescription> m_topic_registry;     //定义一个无序映射（哈希表），用于存储topic name到 topic 的映射

    std::unordered_map<std::string, std::type_index> m_topic_type_map;                  //定义一个无序映射（哈希表），用于存储topic name到 topic message type的映射
    
    bool m_initialized = false;
}

using DdsFactoryPtr = DdsFactoryModel::Ptr;

} // namespace robot
} // namespace yunji

#endif // YUNJI_COMMON_DDS_DDS_FACTORY_MODEL_HPP