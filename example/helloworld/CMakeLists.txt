add_executable(test_publisher 
    publisher.cpp 
    HelloWorldData.cpp
)
target_link_libraries(test_publisher ddscxx)

add_executable(test_subscriber 
    subscriber.cpp 
    HelloWorldData.cpp
)
target_link_libraries(test_subscriber ddscxx)