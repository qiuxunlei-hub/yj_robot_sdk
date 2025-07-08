/*
 * Copyright(c) 2006 to 2021 ZettaScale Technology and others
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>

// 引入 Cyclone DDS 的 C++ API 核心库
#include "dds/dds.hpp"
// 引入通过 IDL 生成的 HelloWorld 数据类型定义（包含序列化/反序列化逻辑）
#include "HelloWorldData.hpp"

using namespace org::eclipse::cyclonedds;  // Cyclone DDS 命名空间

int main() {
    try {
        std::cout << "=== [Subscriber] Create reader." << std::endl;

        // 1. 创建域参与者（DomainParticipant），默认域 ID 为 0
        //    域参与者是 DDS 通信的入口，管理主题、发布者/订阅者等实体
        dds::domain::DomainParticipant participant(domain::default_id());

        // 2. 创建主题（Topic），指定数据类型为 HelloWorldData::Msg，主题名为 "HelloWorldData_Msg"
        //    主题是发布者和订阅者之间的通信桥梁，需确保双方使用相同的主题名和数据类型
        dds::topic::Topic<HelloWorldData::Msg> topic(participant, "HelloWorldData_Msg");

        // 3. 创建订阅者（Subscriber），用于管理数据读取器
        dds::sub::Subscriber subscriber(participant);

        // 4. 创建数据读取器（DataReader），绑定到订阅者和主题
        //    DataReader 是接收数据的核心实体，支持 read()/take() 等操作
        dds::sub::DataReader<HelloWorldData::Msg> reader(subscriber, topic);

        // 5. 轮询等待消息到达（实际项目建议用 Listener 或 WaitSet 替代轮询）
        std::cout << "=== [Subscriber] Wait for message." << std::endl;
        bool poll = true;
        while (poll) {
            // 尝试从读取器获取样本（take() 会移除内部队列中的数据，read() 则保留）
            dds::sub::LoanedSamples<HelloWorldData::Msg> samples = reader.take();

            if (samples.length() > 0) {
                // 遍历所有收到的样本（可能含多条消息）
                for (auto sample_iter = samples.begin(); sample_iter < samples.end(); ++sample_iter) {
                    const HelloWorldData::Msg& msg = sample_iter->data();  // 提取消息内容
                    const dds::sub::SampleInfo& info = sample_iter->info(); // 提取元数据（如时间戳）

                    // 检查样本是否有效（避免处理元数据变更等无效样本）
                    if (info.valid()) {
                        std::cout << "=== [Subscriber] Message received:" << std::endl;
                        std::cout << "    userID  : " << msg.userID() << std::endl;    // 打印消息字段
                        std::cout << "    Message : \"" << msg.message() << "\"" << std::endl;
                        poll = false;  // 示例仅接收一条消息后退出
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 避免 CPU 空转
            }
        }
    } catch (const dds::core::Exception& e) {
        // 捕获 DDS 核心异常（如通信失败、QoS 不兼容等）
        std::cerr << "=== [Subscriber] DDS exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        // 捕获通用 C++ 异常（如内存错误）
        std::cerr << "=== [Subscriber] C++ exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "=== [Subscriber] Done." << std::endl;
    return EXIT_SUCCESS;
}
