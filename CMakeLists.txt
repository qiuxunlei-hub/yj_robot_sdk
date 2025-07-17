# 指定CMake的最低版本要求
cmake_minimum_required(VERSION 3.5)

# 定义项目名称和版本号
project(yunji_sdk VERSION 1.0.0)

## Project Options
# 添加一个选项，用于控制是否构建示例程序
option(BUILD_EXAMPLES "Build examples" ON)

## Set compiler to use c++ 17 features
# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
# 禁用编译器特定的扩展
set(CMAKE_CXX_EXTENSIONS OFF)
# 要求编译器必须支持C++17标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## Choose build type
# 设置默认构建类型为Release
set(default_build_type "Release")
# 如果用户没有指定构建类型，则使用默认的Release
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # 为cmake-gui设置可选的构建类型
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

## Use GNUInstallDirs to install libraries into correct locations on all platforms.
# 包含GNUInstallDirs模块，用于跨平台的安装路径管理
include(GNUInstallDirs)

## Put all binary files into /bin and libraries into /lib
# 设置输出目录：
# 静态库(.a)输出到<build_dir>/lib
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
# 动态库(.so)输出到<build_dir>/lib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
# 可执行文件输出到<build_dir>/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

# 定义项目根目录变量
set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})

## Check system architecture
# 打印当前系统架构信息
message(STATUS "Current system architecture: ${CMAKE_SYSTEM_PROCESSOR}")

# 查找系统线程库
find_package(Threads REQUIRED)

## Import thirdparty libraries
# 添加第三方库目录（该目录应有自己的CMakeLists.txt）
add_subdirectory(thirdparty)

option(BUILD_YUNJI_FROM_SOURCE "Build yunji_sdk from source" ON)

if(BUILD_YUNJI_FROM_SOURCE)

    #编译yunji_sdk静态库
    add_subdirectory(src/yunji)

else()

    # Import Yunji SDK library
    # 设置Yunji SDK库的查找路径（基于系统架构）
    set(Yunji_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR})
    # 查找名为yunji_sdk的预编译库
    find_library(Yunji_SDK_LIB yunji_sdk PATHS ${Yunji_SDK_PATH} NO_DEFAULT_PATH)

    # 检查是否找到库文件
    if (NOT Yunji_SDK_LIB)
        message(FATAL_ERROR "Yunji SDK library for the architecture is not found")
    else ()
        message(STATUS "Yunji SDK library found at: ${Yunji_SDK_LIB}")
    endif ()

    message(STATUS "Importing: ${Yunji_SDK_LIB}")

    # 创建导入的静态库目标
    add_library(yunji_sdk STATIC IMPORTED GLOBAL)
    # 设置导入库的路径
    set_target_properties(yunji_sdk PROPERTIES
            IMPORTED_LOCATION ${Yunji_SDK_LIB})

    # 设置库的依赖关系
    target_link_libraries(yunji_sdk INTERFACE ddsc ddscxx Threads::Threads)

    # 设置库的头文件包含路径
    target_include_directories(yunji_sdk INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)

endif()


# 如果启用了示例构建选项，添加示例目录
if (BUILD_EXAMPLES)
    add_subdirectory(example)
endif ()

## Install the library
# 安装头文件目录
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 安装库文件
install(FILES ${Yunji_SDK_LIB}
    DESTINATION ${CMAKE_INSTALL_LIBDIR})

# 安装目标配置文件
install(FILES cmake/yunji_sdkTargets.cmake
    DESTINATION lib/cmake/yunji_sdk)

# 包含用于创建包配置的辅助函数
include(CMakePackageConfigHelpers)

# 生成包版本文件
write_basic_package_version_file(
    yunji_sdkConfigVersion.cmake
    VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}"
    COMPATIBILITY ExactVersion)

# 配置包配置文件（从模板生成）
configure_file(cmake/yunji_sdkConfig.cmake.in yunji_sdkConfig.cmake @ONLY)

# 安装生成的配置文件
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/yunji_sdkConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/yunji_sdkConfigVersion.cmake"
    DESTINATION lib/cmake/yunji_sdk)