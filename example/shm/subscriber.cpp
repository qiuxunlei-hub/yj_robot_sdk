#include "HelloWorldData.hpp"
#include "dds/dds.hpp"
#include <iostream>
#include <atomic>
#include <thread>       // 提供std::this_thread功能
#include <chrono>       // 提供时间相关功能如std::chrono::seconds
#include <csignal>  // 提供signal()和SIGINT

using namespace org::eclipse::cyclonedds;

std::atomic<bool> running(true);

void signal_handler(int) {
    running = false;
}


// int main() {
//     std::signal(SIGINT, signal_handler);
    
//     try {
//         // 创建DomainParticipant（域0）
//         dds::domain::DomainParticipant participant(domain::default_id());
        
//         // 创建Topic（类型为HelloWorld）
//         dds::topic::Topic<ThroughputModule::DataType_262144> topic(participant, "HelloWorldTopic");
        
//         // 创建Subscriber
//         dds::sub::Subscriber subscriber(participant);
        
//         // 配置DataReader Qos
//         dds::sub::qos::DataReaderQos reader_qos;
//         reader_qos << dds::core::policy::Reliability::Reliable()
//                    << dds::core::policy::History::KeepAll();
        
//         // 创建DataReader
//         //dds::sub::DataReader<ThroughputModule::DataType_262144> reader(subscriber, topic, reader_qos);

//         dds::sub::DataReader<ThroughputModule::DataType_262144> reader(subscriber, topic);


//         while (running) {
//             // 尝试从读取器获取样本（take() 会移除内部队列中的数据，read() 则保留）
//             dds::sub::LoanedSamples<ThroughputModule::DataType_262144> samples = reader.take();

//             if (samples.length() > 0) {
//                 // 遍历所有收到的样本（可能含多条消息）
//                 for (auto sample_iter = samples.begin(); sample_iter < samples.end(); ++sample_iter) {
//                     const ThroughputModule::DataType_262144& msg = sample_iter->data();  // 提取消息内容
//                     const dds::sub::SampleInfo& info = sample_iter->info(); // 提取元数据（如时间戳）

//                     // 检查样本是否有效（避免处理元数据变更等无效样本）
//                     if (info.valid()) {
//                         // std::cout << "=== [Subscriber] Message received:" << std::endl;
//                         // std::cout << "    userID  : " << msg.userID() << std::endl;    // 打印消息字段
//                         // std::cout << "    Message : \"" << msg.message() << "\"" << std::endl;

//                         uint64_t send_us  = msg.count();
                       
//                         struct timespec ts;
                        
//                         if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1){
//                             perror("clock_gettime");
//                             return -1;
//                         }
//                         uint64_t timestamp = ts.tv_sec * 1000000 + ts.tv_nsec / 1000; // 转换为微秒
                        
//                         // 计算延迟（得到duration对象）
//                         auto latency_duration = timestamp - send_us;
                        
//                         std::cout << "Latency: " << latency_duration << " us" << std::endl;
//                     }
//                 }
//             } else {
//                 //std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 避免 CPU 空转
//             }
//         }
//     } catch (const dds::core::Exception& e) {
//         std::cerr << "DDS Exception: " << e.what() << std::endl;
//         return 1;
//     }
    
//     std::cout << "Subscriber exiting." << std::endl;
//     return 0;
// }


// 自定义监听器类
class DataReaderListener : public dds::sub::NoOpDataReaderListener<ThroughputModule::DataType_262144> {
public:
    void on_data_available(dds::sub::DataReader<ThroughputModule::DataType_262144>& reader) override {
        // 处理新到达的数据
        dds::sub::LoanedSamples<ThroughputModule::DataType_262144> samples = reader.take();

        if (samples.length() > 0) {
            for (auto sample_iter = samples.begin(); sample_iter < samples.end(); ++sample_iter) {
                const ThroughputModule::DataType_262144& msg = sample_iter->data();
                const dds::sub::SampleInfo& info = sample_iter->info();

                if (info.valid()) {
                    uint64_t send_us = msg.count();
                    
                    // struct timespec ts;
                    // if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1){
                    //         perror("clock_gettime");
                    //     }
                    
                    // uint64_t timestamp = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

                    dds_time_t now = dds_time();

                    // 转换为微秒/毫秒/秒
                    uint64_t micros = now/1000; // 微秒

                    auto latency_duration = micros - send_us;
                    
                    std::cout << "Latency: " << latency_duration << " us" << std::endl;
                }
            }
        }
    }
};


int main() {
    std::signal(SIGINT, signal_handler);
    
    try {
        // 创建DomainParticipant（域0）
        dds::domain::DomainParticipant participant(domain::default_id());
        
        // 创建Topic（类型为HelloWorld）
        dds::topic::Topic<ThroughputModule::DataType_262144> topic(participant, "HelloWorldTopic");
        
        // 创建Subscriber
        dds::sub::Subscriber subscriber(participant);
        
        // 配置DataReader Qos
        dds::sub::qos::DataReaderQos reader_qos;
        reader_qos << dds::core::policy::Reliability::Reliable()
                   << dds::core::policy::History::KeepAll();
        
        // 创建DataReader
        //dds::sub::DataReader<ThroughputModule::DataType_262144> reader(subscriber, topic, reader_qos);

        // 创建监听器实例
        DataReaderListener listener;

        dds::sub::DataReader<ThroughputModule::DataType_262144> reader(subscriber, topic);

        reader.listener(&listener, dds::core::status::StatusMask::data_available());


        // 主线程只需等待，不需要轮询
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 清理时移除监听器
        reader.listener(nullptr, dds::core::status::StatusMask::none());

    } catch (const dds::core::Exception& e) {
        std::cerr << "DDS Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Subscriber exiting." << std::endl;
    return 0;
}
