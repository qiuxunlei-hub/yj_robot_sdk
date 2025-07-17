#include "yunji/robot/dds_bridge/dds_bridge_publisher.hpp"
#include "yunji/idl/HelloWorldData.hpp"

#define TOPIC "TopicHelloWorld"

using namespace yunji::robot;

int main()
{
    BridgeFactory::Instance()->Init(0, "");
    BridgePublisher<HelloWorldData::Msg> publisher(TOPIC);

    publisher.InitBridge();

    while (true)
    {
        HelloWorldData::Msg msg(0, "HelloWorld.");
        publisher.Write(msg);
        sleep(1);
    }

    return 0;
}