# 查找系统线程库（pthread）
find_package(Threads REQUIRED)

# 打印正在导入的 ddsc 库路径信息
message(STATUS "Importing: ${CMAKE_CURRENT_LIST_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}/libddsc.so")

# 创建 ddsc 共享库的导入目标
add_library(ddsc SHARED IMPORTED GLOBAL)
# 设置导入库的实际路径
set_target_properties(ddsc PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}/libddsc.so)
# 禁用 SONAME（适用于没有版本号的库）
set_target_properties(ddsc PROPERTIES IMPORTED_NO_SONAME TRUE)
# 设置库的依赖关系（链接线程库）
target_link_libraries(ddsc INTERFACE Threads::Threads)
# 设置链接库的搜索路径
target_link_directories(ddsc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR})
# 设置头文件包含路径（构建时和安装后）
target_include_directories(ddsc INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  # 构建时使用源码目录
        $<INSTALL_INTERFACE:include>)                           # 安装后使用标准include目录

# 打印正在导入的 ddscxx 库路径信息
message(STATUS "Importing: ${CMAKE_CURRENT_LIST_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}/libddscxx.so")

# 创建 ddscxx 共享库的导入目标
add_library(ddscxx SHARED IMPORTED GLOBAL)
# 设置导入库的实际路径
set_target_properties(ddscxx PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}/libddscxx.so)
# 禁用 SONAME
set_target_properties(ddscxx PROPERTIES IMPORTED_NO_SONAME TRUE)
# 设置库的依赖关系（链接线程库）
target_link_libraries(ddscxx INTERFACE Threads::Threads)
# 设置链接库的搜索路径
target_link_directories(ddscxx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR})
# 设置头文件包含路径（包含额外的 ddscxx 目录）
target_include_directories(ddscxx INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>          # 主头文件目录
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/iceoryx/v2.0.2>   # ddscxx 特定头文件
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ddscxx>  
        $<INSTALL_INTERFACE:include>                                    # 安装后的主头文件目录
        $<INSTALL_INTERFACE:include/ddscxx>)                            # 安装后的 ddscxx 头文件目录

## 安装目标
# 收集指定架构目录下的所有库文件
file(GLOB DDS_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}/*")

# 安装所有库文件到标准库目录
install(FILES ${DDS_LIBRARIES} DESTINATION ${CMAKE_INSTALL_LIBDIR})

# 安装 include 目录下的所有头文件
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
