cmake_minimum_required(VERSION 3.5)
project(yunji_sdk)

# 递归查找所有 .cpp 和 .c 文件（包括子目录）
file(GLOB_RECURSE YUNJI_SOURCES 
    "*.cpp" 
    "*.c"
)

# 设置静态库输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR})

# 创建静态库
add_library(yunji_sdk STATIC ${YUNJI_SOURCES})

# 设置头文件路径（包含子目录）
target_include_directories(yunji_sdk PUBLIC
    ${PROJECT_ROOT_DIR}/include
)

# 链接其他依赖库（如 ddsc、Threads）
target_link_libraries(yunji_sdk PRIVATE ddsc ddscxx Threads::Threads)

