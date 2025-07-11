#include "HelloWorldData.hpp"
#include "dds/dds.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>       // 提供时间相关功能如std::chrono::seconds
#include <csignal>  // 提供signal()和SIGINT

using namespace org::eclipse::cyclonedds;

std::atomic<bool> running(true);

void signal_handler(int) {
    running = false;
}

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
        
        // 创建Publisher
        dds::pub::Publisher publisher(participant);
        
        // 配置DataWriter Qos - 使用共享内存传输
        dds::pub::qos::DataWriterQos writer_qos;
        writer_qos << dds::core::policy::Reliability::BestEffort()  // 改用BestEffort
           << dds::core::policy::History::KeepLast(1)       // 减少历史记录
           << dds::core::policy::ResourceLimits(10, 10)    // 限制资源
           << dds::core::policy::TransportPriority(10);     // 提高传输优先级
        
        // 创建DataWriter
        dds::pub::DataWriter<ThroughputModule::DataType_262144> writer(publisher, topic, writer_qos);
        //dds::pub::DataWriter<ThroughputModule::DataType_262144> writer(publisher, topic);

        // 创建Subscriber
        dds::sub::Subscriber subscriber(participant);
        
        // 配置DataReader Qos
        dds::sub::qos::DataReaderQos reader_qos;
        reader_qos << dds::core::policy::Reliability::BestEffort()
           << dds::core::policy::History::KeepLast(1);
        
        // 创建DataReader
        dds::sub::DataReader<ThroughputModule::DataType_262144> reader(subscriber, topic, reader_qos);

        // 创建监听器实例
        DataReaderListener listener;

        //dds::sub::DataReader<ThroughputModule::DataType_262144> reader(subscriber, topic);

        reader.listener(&listener, dds::core::status::StatusMask::data_available());

        
        std::cout << "Publisher ready. Sending messages (Ctrl+C to exit)..." << std::endl;
        
        // 写入数据
        int count = 0;

        ThroughputModule::DataType_262144 msg;
        
        for(volatile int i = 0 ; i<255; i++)
        {
            msg.payload()[i] = 1;
        }

        while (running) {
            
            // struct timespec ts;

            // if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1){
            //     perror("clock_gettime");
            //     return -1;
            // }
            // uint64_t timestamp = ts.tv_sec * 1000000 + ts.tv_nsec / 1000; // 转换为微秒

            // 获取当前时间
            dds_time_t now = dds_time();

            msg.count(now/1000);

            writer.write(msg);
            
            // std::cout << "Sent: " << msg.message() 
            //           << std::endl;
            
           std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 避免 CPU 空转
        }
    } catch (const dds::core::Exception& e) {
        std::cerr << "DDS Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Publisher exiting." << std::endl;
    return 0;
}