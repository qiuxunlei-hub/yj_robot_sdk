#include "yunji/robot/dds_bridge/dds_bridge_subscriber.hpp"
#include "yunji/idl/HelloWorldData.hpp"

#define TOPIC "TopicHelloWorld"

using namespace yunji::robot;

void Handler(const HelloWorldData::Msg &msg)
{

    //const HelloWorldData::Msg* pm = (const HelloWorldData::Msg*)msg;

    //std::cout << "userID:" << pm->userID() << ", message:" << pm->message() << std::endl;

    std::cout << "userID:" << msg.userID() << ", message:" << msg.message() << std::endl;

}

int main()
{
    BridgeFactory::Instance()->Init(0);
    BridgeSubscriber<HelloWorldData::Msg> subscriber(TOPIC);
    subscriber.InitBridge(Handler);

    while (true)
    {
        sleep(10);
    }

    return 0;
}