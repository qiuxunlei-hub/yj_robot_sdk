#ifndef __YJ_ROBOT_SDK_CHANNEL_FACTORY_HPP__
#define __YJ_ROBOT_SDK_CHANNEL_FACTORY_HPP__

/**
 * @file bridge_factory.hpp
 * @brief DDS抽象层代理封装类
 * @copyright Copyright (c) 2025 YunJi Robotics. All rights reserved.
 */

#include <yunji/common/dds/dds_factory_model.hpp> 

namespace yunji
{

namespace robot
{

/**
 * @brief DDS通道类型别名（值语义）
 * @tparam MSG 消息类型（需支持DDS序列化）
 */
template<typename MSG>
using Bridge = unitree::common::DdsTopicChannel<MSG>;


template<typename MSG>
using BridgePtr = unitree::common::DdsTopicChannelPtr<MSG>;

/**
 * @class BridgeFactory
 * @brief DDS代理类（单例模式）
 * 
 * 提供以下核心功能：
 * 1. 初始化DDS域和网络接口
 * 2. 创建发布/订阅通道
 * 3. 管理通道生命周期
 */
class BridgeFactory {
public:
    /**
     * @brief 获取单例实例
     * @return ChannelFactory* 单例指针
     * @note 线程安全的懒汉式单例（C++11标准保证）
     */
    static BridgeFactory* Instance()
    {
        static BridgeFactory inst;
        return &inst;
    }

    /**
     * @brief 初始化DDS通信层（通过DomainID）
     * @param domainId DDS域ID（建议范围：0~232）
     * @param networkInterface 绑定的网络接口名（如"eth0"，空字符串为自动选择）
     */
    void Init(int32_t domainId, const std::string& networkInterface = "");

    /**
     * @brief 初始化DDS通信层（通过配置文件）
     * @param configFileName XML配置文件路径（如"config/cyclonedds.xml"）
     */
    void Init(const std::string& configFileName = "");

    /**
     * @brief 释放所有DDS资源
     * @warning 调用后需重新Init才能使用
     */
    void Release();

    /**
     * @brief 创建发布通道（模板方法）
     * @tparam MSG 消息类型
     * @param name 通道名称（对应DDS Topic）
     * @return ChannelPtr<MSG> 通道智能指针
     * @throws DdsException 创建失败时抛出
     */
    template<typename MSG>
    BridgePtr<MSG> CreateSendBridge(const std::string& name)
    {
        
        BridgePtr<MSG> bridgePtr = mDdsFactoryPtr->CreateTopicChannel<MSG>(name);
        
        mDdsFactoryPtr->SetWriter(bridgePtr);

        return bridgePtr;
    }

    /**
     * @brief 创建订阅通道（模板方法）
     * @tparam MSG 消息类型
     * @param name 通道名称
     * @param callback 数据到达回调函数 void(const void* data)
     * @param queuelen 消息队列长度（0=默认值）
     * @return ChannelPtr<MSG> 通道智能指针
     */
    template<typename MSG>
    BridgePtr<MSG> CreateRecvBridge(const std::string& name, 
                                    std::function<void(const void*)> callback, 
                                    int32_t queuelen = 0)
    {
        
        BridgePtr<MSG> bridgePtr = mDdsFactoryPtr->CreateTopicChannel<MSG>(name);
        
        mDdsFactoryPtr->SetReader(bridgePtr, callback, queuelen);
        return bridgePtr;
    }

public:
    ~BridgeFactory()
    {
        Release();
    }

private:
    BridgeFactory();  

private:
    bool mInited;                     
    common::DdsFactoryModelPtr mDdsFactoryPtr;  
    common::Mutex mMutex;             
};

}
}

#endif//__YJ_ROBOT_SDK_CHANNEL_FACTORY_HPP__