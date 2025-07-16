#ifndef __YJ_ROBOT_SDK_CHANNEL_FACTORY_HPP__
#define __YJ_ROBOT_SDK_CHANNEL_FACTORY_HPP__

/**
 * @file bridge_factory.hpp
 * @brief DDS抽象层代理封装类
 * @copyright Copyright (c) 2025 YunJi Robotics. All rights reserved.
 */

#include <dds/dds.hpp>  // CycloneDDS核心头文件
#include <thread>  // 添加这行

namespace yunji
{

namespace robot
{


class BridgeFactory {

public:

    static BridgeFactory* Instance() {

        static BridgeFactory instance;

        return &instance;
    }

    /**
     * @brief 初始化DDS通信层（通过DomainID）
     * @param domainId DDS域ID（建议范围：0~232）
     * @param networkInterface 绑定的网络接口名（如"eth0"，空字符串为自动选择）
     */
    void Init(int domain_id, const std::string& network_interface = "");

    /**
     * @brief 初始化DDS通信层（通过配置文件）
     * @param configFileName XML配置文件路径（如"config/cyclonedds.xml"）
     */
    void Init(const std::string& config_path = "");




    std::shared_ptr<dds::domain::DomainParticipant> GetParticipant() const {

        return participant_;

    }

private:

    BridgeFactory() = default;

    std::shared_ptr<dds::domain::DomainParticipant> participant_;
};

}
}

#endif//__YJ_ROBOT_SDK_CHANNEL_FACTORY_HPP__