
add_executable(shm_publisher 
    publisher.cpp 
    HelloWorldData.cpp
)
target_link_libraries(shm_publisher ddscxx ddsc)

add_executable(shm_subscriber 
    subscriber.cpp 
    HelloWorldData.cpp
)
target_link_libraries(shm_subscriber ddscxx ddsc)