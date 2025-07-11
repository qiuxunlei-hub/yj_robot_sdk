#include "yunji/robot/dds_bridge/dds_factory_model.hpp"
#include <dds/dds.hpp>
#include <stdexcept>

namespace yunji {
namespace common {

DdsFactoryModel::DdsFactoryModel() {
    // 默认构造，延迟初始化
}

DdsFactoryModel::~DdsFactoryModel() {
    Release();
}

void DdsFactoryModel::Init(int32_t domainId, const std::string& networkInterface) {
    if (mInitialized) {
        throw std::runtime_error("DdsFactoryModel already initialized");
    }

    try {
        // 设置环境变量（如果指定了网络接口）
        if (!networkInterface.empty()) {
            setenv("CYCLONEDDS_URI", 
                  ("<CycloneDDS><Domain><General><NetworkInterfaceAddress>"
                   + networkInterface + "</NetworkInterfaceAddress></General></Domain></CycloneDDS>").c_str(), 
                  1);
        }
        
        // 创建域参与者
        mParticipant = dds::domain::DomainParticipant(domainId);
        
        // 创建发布者和订阅者
        mPublisher = dds::pub::Publisher(mParticipant);
        mSubscriber = dds::sub::Subscriber(mParticipant);
        
        mInitialized = true;
    } catch (const dds::core::Exception& e) {
        throw std::runtime_error(std::string("DDS initialization failed: ") + e.what());
    }
}

void DdsFactoryModel::Init(const std::string& configFileName) {
    if (mInitialized) {
        throw std::runtime_error("DdsFactoryModel already initialized");
    }

    try {
        // 通过配置文件初始化
        setenv("CYCLONEDDS_URI", ("file://" + configFileName).c_str(), 1);
        
        // 创建域参与者（使用默认域）
        mParticipant = dds::domain::DomainParticipant(dds::domain::default_id());
        
        // 创建发布者和订阅者
        mPublisher = dds::pub::Publisher(mParticipant);
        mSubscriber = dds::sub::Subscriber(mParticipant);
        
        mInitialized = true;
    } catch (const dds::core::Exception& e) {
        throw std::runtime_error(std::string("DDS config initialization failed: ") + e.what());
    }
}

void DdsFactoryModel::Release() {
    if (mInitialized) {
        // 清除主题映射
        mTopicMap.clear();
        
        // 释放DDS资源（按正确顺序）
        mSubscriber = dds::sub::Subscriber(nullptr);
        mPublisher = dds::pub::Publisher(nullptr);
        mParticipant = dds::domain::DomainParticipant(nullptr);
        
        mInitialized = false;
    }
}

} // namespace common
} // namespace unitree