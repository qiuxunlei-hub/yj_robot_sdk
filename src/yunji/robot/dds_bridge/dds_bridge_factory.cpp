/**
 * @file bridge_factory.cpp
 * @brief DDS通道工厂实现文件
 * @note 实现BridgeFactory类的具体功能
 */
#include "yunji/robot/dds_bridge/dds_bridge_factory.hpp"

namespace yunji {
namespace robot {

/**
 * @brief ChannelFactory构造函数
 * @details 初始化成员变量：
 * - mInited: 设置为false表示未初始化
 * - mDdsFactoryPtr: 创建DdsFactoryModel的共享指针
 */
BridgeFactory::BridgeFactory() : 
    mInited(false),
    mDdsFactoryPtr(std::make_shared<unitree::common::DdsFactoryModel>()) {}

/**
 * @brief ChannelFactory析构函数
 * @details 自动释放DDS资源
 */
BridgeFactory::~BridgeFactory() {
    Release();
}

void BridgeFactory::Init(int32_t domainId, const std::string& networkInterface) {
    unitree::common::LockGuard lock(mMutex);
    
    if (mInited) {
        throw unitree::common::DdsException("BridgeFactory already initialized");
    }

    try {
        mDdsFactoryPtr->Init(domainId, networkInterface);
        mInited = true;
    } catch (const std::exception& e) {
        throw unitree::common::DdsException(std::string("DDS init failed: ") + e.what());
    }
}

/**
 * @brief 初始化DDS通信层（通过配置文件）
 * @param configFileName 配置文件路径
 * @throw DdsException 初始化失败时抛出异常
 * @note 线程安全，使用互斥锁保护
 */
void BridgeFactory::Init(const std::string& configFileName) {
    unitree::common::LockGuard lock(mMutex);
    
    if (mInited) {
        throw unitree::common::DdsException("BridgeFactory already initialized");
    }

    try {
        mDdsFactoryPtr->Init(configFileName);
        mInited = true;
    } catch (const std::exception& e) {
        throw unitree::common::DdsException(std::string("DDS config load failed: ") + e.what());
    }
}

/**
 * @brief 释放DDS资源
 * @note 线程安全，使用互斥锁保护
 * @details 如果已初始化，则调用底层Release()并重置状态
 */
void BridgeFactory::Release() {
    unitree::common::LockGuard lock(mMutex);
    
    if (mInited) {
        mDdsFactoryPtr->Release();
        mInited = false;
    }
}

} // namespace robot
} // namespace yunji
