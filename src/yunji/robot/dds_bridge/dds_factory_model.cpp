#include "yunji/common/dds/dds_factory_model.hpp"
#include <dds/dds.hpp>
#include <stdexcept>
#include <cstdlib>
#include <atomic>   
#include <chrono>  

namespace yunji {
namespace robot {

// 内部实现类定义
template<typename MsgType>
class DdsFactory::TopicChannelImpl : public DdsTopicChannel<MsgType> {
public:
    TopicChannelImpl(DdsFactory::Ptr factory, const std::string& topic_name)
        : m_factory(std::move(factory)), m_topic_name(topic_name) {}        //std::move(factory)将shared_ptr所有权移动为m_factory而非拷贝，避免了智能指针的引用计数操作
    
    bool Write(const MsgType& message) override {
        if (!m_writer) {
            throw std::logic_error("Writer not enabled for topic: " + m_topic_name);
        }
        
        try {
            m_writer->write(message);
            return true;
        } catch (const dds::core::Exception& e) {
            // 实际项目中应记录日志
            return false;
        }
    }
    
    void create_writer() {
        if (!m_writer) {
            auto topic = m_factory->get_or_create_topic<MsgType>(m_topic_name);
            
            dds::pub::qos::DataWriterQos qos;
            qos << dds::core::policy::Reliability::Reliable();
            
            m_writer = std::make_unique<dds::pub::DataWriter<MsgType>>(
                m_factory->m_publisher, topic, qos);
        }
    }
    
    void create_reader(std::function<void(const MsgType&)> callback, size_t queue_capacity) {
        if (!m_reader) {
            auto topic = m_factory->get_or_create_topic<MsgType>(m_topic_name);
            
            dds::sub::qos::DataReaderQos qos;
            qos << dds::core::policy::Reliability::Reliable();
            
            if (queue_capacity > 0) {
                qos << dds::core::policy::History::KeepLast(queue_capacity);
            }
            
            m_reader = std::make_unique<dds::sub::DataReader<MsgType>>(
                m_factory->m_subscriber, topic, qos);
            
            // 设置数据到达监听器
            m_reader->listener([callback](dds::sub::DataReader<MsgType>& reader) {
                auto samples = reader.take();
                for (const auto& sample : samples) {
                    if (sample.info().valid()) {
                        last_data_time_ = get_current_time_micros();

                        callback(sample.data());
                    }
                }
            }, dds::core::status::StatusMask::data_available());
        }
    }

    // 添加获取最后数据时间的方法
    uint64_t GetLastDataAvailableTime() override {
        return last_data_time_;
    }

private:
    // 获取当前时间（微秒）
    static uint64_t get_current_time_micros() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
    }

private:
    DdsFactory::Ptr m_factory;

    std::string m_topic_name;

    std::unique_ptr<dds::pub::DataWriter<MsgType>> m_writer;        //dds写入器指针

    std::unique_ptr<dds::sub::DataReader<MsgType>> m_reader;        //dds读取器指针

    std::atomic<uint64_t> last_data_time_;  // 原子操作保证线程安
}

DdsFactory::DdsFactory() {
    // 延迟初始化设计
}

DdsFactory::~DdsFactory() {
    shutdown();
}

void DdsFactory::initialize(int32_t domain_id, const std::string& network_interface) {
    if (m_initialized) {
        throw std::runtime_error("DDS factory already initialized");
    }

    try {
        // 配置网络接口（如果指定）
        if (!network_interface.empty()) {
            const std::string config_xml = 
                "<CycloneDDS>"
                "  <Domain>"
                "    <General>"
                "      <NetworkInterfaceAddress>" + network_interface + 
                "</NetworkInterfaceAddress>"
                "    </General>"
                "  </Domain>"
                "</CycloneDDS>";
            
            ::setenv("CYCLONEDDS_URI", config_xml.c_str(), 1);
        }
        
        // 创建域参与者
        m_participant = dds::domain::DomainParticipant(domain_id);
        
        // 创建发布者和订阅者
        m_publisher = dds::pub::Publisher(m_participant);
        m_subscriber = dds::sub::Subscriber(m_participant);
        
        m_initialized = true;
        
    } catch (const dds::core::Exception& e) {
        throw std::runtime_error("DDS initialization failed: " + std::string(e.what()));
    }
}

void DdsFactory::initialize(const std::string& config_path) {
    if (m_initialized) {
        throw std::runtime_error("DDS factory already initialized");
    }

    try {
        // 设置配置文件路径
        const std::string uri = "file://" + config_path;
        ::setenv("CYCLONEDDS_URI", uri.c_str(), 1);
        
        // 使用默认域ID创建参与者
        m_participant = dds::domain::DomainParticipant(dds::domain::default_id());
        
        // 创建发布者和订阅者
        m_publisher = dds::pub::Publisher(m_participant);
        m_subscriber = dds::sub::Subscriber(m_participant);
        
        m_initialized = true;
        
    } catch (const dds::core::Exception& e) {
        throw std::runtime_error("DDS config initialization failed: " + std::string(e.what()));
    }
}

void DdsFactory::shutdown() {
    if (!m_initialized) return;

    // 按依赖顺序释放资源
    {
        std::lock_guard<std::mutex> lock(m_topic_mutex);
        m_topic_registry.clear();
        m_topic_type_map.clear();
    }
    
    // 释放订阅者/发布者
    m_subscriber = dds::sub::Subscriber(nullptr);
    m_publisher = dds::pub::Publisher(nullptr);
    
    // 最后释放域参与者
    m_participant = dds::domain::DomainParticipant(nullptr);
    
    m_initialized = false;
}

template<typename MsgType>
dds::topic::Topic<MsgType> DdsFactory::get_or_create_topic(const std::string& topic_name) {
    std::lock_guard<std::mutex> lock(m_topic_mutex);
    const std::type_index msg_type = typeid(MsgType);
    
    // 检查是否已注册
    if (auto it = m_topic_type_map.find(topic_name); it != m_topic_type_map.end()) {
        if (it->second != msg_type) {
            throw std::runtime_error("Topic type conflict for '" + topic_name + 
                                     "'. Existing type differs from requested.");
        }
        
        // 安全转换
        try {
            return dds::topic::topic_cast<dds::topic::Topic<MsgType>>(
                m_topic_registry.at(topic_name)
            );
        } catch (const dds::core::InvalidDowncastError&) {
            throw std::runtime_error("Topic type conversion failed for: " + topic_name);
        }
    }
    
    // 创建新主题
    dds::topic::Topic<MsgType> topic(m_participant, topic_name);
    
    // 注册主题
    m_topic_registry.emplace(topic_name, topic);
    m_topic_type_map.emplace(topic_name, msg_type);
    
    return topic;
}

// 模板成员函数实现
template<typename MsgType>
DdsTopicChannelPtr<MsgType> DdsFactory::create_topic_channel(const std::string& topic_name) {
    return std::make_shared<TopicChannelImpl<MsgType>>(shared_from_this(), topic_name);
}

template<typename MsgType>
void DdsFactory::enable_writer(DdsTopicChannelPtr<MsgType> channel) {
    auto impl = std::dynamic_pointer_cast<TopicChannelImpl<MsgType>>(channel);      //动态类型转换将DdsTopicChannel类型指针转换为TopicChannelImpl，如果channel为TopicChannelImpl<MsgType>类型的shared_ptr则impl安全持有该shared_ptr否则impl为null
    if (impl) {
        impl->create_writer();                                                      //创建topic写入器
    } else {
        throw std::invalid_argument("Invalid channel type for writer enablement");
    }
}

template<typename MsgType>
void DdsFactory::enable_reader(DdsTopicChannelPtr<MsgType> channel, 
                              std::function<void(const MsgType&)> data_callback,
                              size_t queue_capacity) {
    auto impl = std::dynamic_pointer_cast<TopicChannelImpl<MsgType>>(channel);
    if (impl) {
        impl->create_reader(data_callback, queue_capacity);
    } else {
        throw std::invalid_argument("Invalid channel type for reader enablement");
    }
}

} // namespace robot
} // namespace yunji