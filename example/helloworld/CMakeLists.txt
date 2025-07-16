add_executable(test_publisher 
    publisher.cpp 
    HelloWorldData.cpp
)
target_link_libraries(test_publisher yunji_sdk ddscxx ddsc)

add_executable(test_subscriber 
    subscriber.cpp 
    HelloWorldData.cpp
)
target_link_libraries(test_subscriber yunji_sdk ddscxx ddsc)