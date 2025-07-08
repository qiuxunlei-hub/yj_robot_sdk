/*
 * 版权声明：使用 Eclipse Public License v2.0 或 Eclipse Distribution License v1.0
 * 涉及公司：ZettaScale Technology 等
 */
#include <cstdlib>      // 标准库函数
#include <iostream>     // 输入输出流
#include <chrono>       // 时间处理
#include <thread>       // 线程支持

/* 包含 DDS 的 C++ API */
#include "dds/dds.hpp"

/* 包含数据类型和特定特征（通常由 IDL 生成） */
#include "HelloWorldData.hpp"

// 使用 Cyclone DDS 的命名空间
using namespace org::eclipse::cyclonedds;

int main() {
    try {
        std::cout << "=== [Publisher] Create writer." << std::endl;

        /* 1. 创建域参与者 (DomainParticipant) */
        // 在默认域上创建域参与者（DDS 应用程序的入口点）
        dds::domain::DomainParticipant participant(domain::default_id());

        /* 2. 创建主题 (Topic) */
        // 定义要发布的数据类型（HelloWorldData::Msg）和主题名称
        dds::topic::Topic<HelloWorldData::Msg> topic(participant, "HelloWorldData_Msg");

        /* 3. 创建发布者 (Publisher) */
        // 发布者用于管理数据写入器
        dds::pub::Publisher publisher(participant);

        /* 4. 创建数据写入器 (DataWriter) */
        // 用于实际发布消息的对象
        dds::pub::DataWriter<HelloWorldData::Msg> writer(publisher, topic);

        /* 5. 等待订阅者连接 */
        // 这是一个简单的轮询等待，实际应用中推荐使用监听器或等待集
        std::cout << "=== [Publisher] Waiting for subscriber." << std::endl;
        while (writer.publication_matched_status().current_count() == 0) {
            // 每20毫秒检查一次是否有订阅者连接
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        /* 6. 创建并发送消息 */
        // 创建消息对象（ID:1, 内容:"Hello World"）
        HelloWorldData::Msg msg(1, "Hello World");

        // 发布消息
        std::cout << "=== [Publisher] Write sample." << std::endl;
        writer.write(msg);

        /* 7. 等待消息被接收 */
        // 再次轮询等待，直到没有匹配的订阅者
        // 在标准配置下，删除写入器会处理所有相关消息
        std::cout << "=== [Publisher] Waiting for sample to be accepted." << std::endl;
        while (writer.publication_matched_status().current_count() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    catch (const dds::core::Exception& e) {
        // 捕获并处理 DDS 核心异常
        std::cerr << "=== [Publisher] Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;  // 异常退出
    }

    std::cout << "=== [Publisher] Done." << std::endl;
    return EXIT_SUCCESS;  // 正常退出
}