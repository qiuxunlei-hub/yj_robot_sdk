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
    mDdsFactoryPtr(std::make_shared<yunji::robot::DdsFactory>()) {}

/**
 * @brief ChannelFactory析构函数
 * @details 自动释放DDS资源
 */
BridgeFactory::~BridgeFactory() {
    Release();
}

void BridgeFactory::Init(int32_t domainId, const std::string& networkInterface) {
    
    if (mInited) {
        throw yunji::robot::DdsException("BridgeFactory already initialized");
    }

    try {
        mDdsFactoryPtr->initialize(domainId, networkInterface);
        mInited = true;
    } catch (const std::exception& e) {
        throw yunji::robot::DdsException(std::string("DDS init failed: ") + e.what());
    }
}

/**
 * @brief 初始化DDS通信层（通过配置文件）
 * @param configFileName 配置文件路径
 * @throw DdsException 初始化失败时抛出异常
 * @note 线程安全，使用互斥锁保护
 */
void BridgeFactory::Init(const std::string& configFileName) {
    yunji::robot::LockGuard lock(mMutex);
    
    if (mInited) {
        throw yunji::robot::DdsException("BridgeFactory already initialized");
    }

    try {
        mDdsFactoryPtr->initialize(configFileName);
        mInited = true;
    } catch (const std::exception& e) {
        throw yunji::robot::DdsException(std::string("DDS config load failed: ") + e.what());
    }
}

/**
 * @brief 释放DDS资源
 * @note 线程安全，使用互斥锁保护
 * @details 如果已初始化，则调用底层Release()并重置状态
 */
void BridgeFactory::Release() {

    if (mInited) {
        mDdsFactoryPtr->shutdown();
        mInited = false;
    }
}

/**
 * @brief 创建发布通道（模板方法）
 * @tparam MSG 消息类型
 * @param name 通道名称（对应DDS Topic）
 * @return ChannelPtr<MSG> 通道智能指针
 * @throws DdsException 创建失败时抛出
 */
template<typename MSG>
BridgePtr<MSG> BridgeFactory::CreateSendBridge(const std::string& name)
{
    
    BridgePtr<MSG> bridgePtr = mDdsFactoryPtr->create_topic_channel<MSG>(name);
    
    mDdsFactoryPtr->enable_writer<MSG>(bridgePtr);

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
    
    BridgePtr<MSG> bridgePtr = mDdsFactoryPtr->create_topic_channel<MSG>(name);
    
    mDdsFactoryPtr->enable_reader<MSG>(bridgePtr, callback, queuelen);
    return bridgePtr;
}

} // namespace robot
} // namespace yunji
