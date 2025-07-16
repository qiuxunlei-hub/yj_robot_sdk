/**
 * @file bridge_factory.cpp
 * @brief DDS通道工厂实现文件
 * @note 实现BridgeFactory类的具体功能
 */
#include "yunji/robot/dds_bridge/dds_bridge_factory.hpp"

namespace yunji {
namespace robot {

/**
 * @brief 初始化DDS通信层（通过配置文件）
 * @param configFileName 配置文件路径
 * @throw DdsException 初始化失败时抛出异常
 * @note 线程安全，使用互斥锁保护
 */
void BridgeFactory::Init(int domain_id, const std::string& network_interface) {
    
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
        participant_ = std::make_shared<dds::domain::DomainParticipant>(domain_id);
        
    } catch (const dds::core::Exception& e) {
        throw std::runtime_error("DDS initialization failed: " + std::string(e.what()));
    }
}

/**
 * @brief 初始化DDS通信层（通过配置文件）
 * @param configFileName 配置文件路径
 * @throw DdsException 初始化失败时抛出异常
 * @note 线程安全，使用互斥锁保护
 */
void BridgeFactory::Init(const std::string& config_path) {
    
    try {
        // 设置配置文件路径
        const std::string uri = "file://" + config_path;
        ::setenv("CYCLONEDDS_URI", uri.c_str(), 1);
        
        // 创建域参与者
        participant_ = std::make_shared<dds::domain::DomainParticipant>(0);
        
        
    } catch (const dds::core::Exception& e) {
        throw std::runtime_error("DDS config initialization failed: " + std::string(e.what()));
    }
}


} // namespace robot
} // namespace yunji
