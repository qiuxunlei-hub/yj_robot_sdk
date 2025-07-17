# 检查CMake版本是否低于3.5.0，若是则报错终止
if(CMAKE_VERSION VERSION_LESS 3.5.0)
  message(FATAL_ERROR "This file relies on consumers using CMake 3.5.0 or greater.")
endif()

# 保存当前CMake策略状态并设置兼容版本为2.6
cmake_policy(PUSH)
cmake_policy(VERSION 2.6)  # 注释：确保兼容旧版CMake的target_link_libraries等命令

# 设置导入文件格式版本（CMake内部使用）
set(CMAKE_IMPORT_FILE_VERSION 1)

# 防止目标重复导入的保护机制
# 初始化变量用于跟踪目标定义状态
set(_targetsDefined)
set(_targetsNotDefined)
set(_expectedTargets)

# 定义期望导入的目标列表（此处只有yunji_sdk）
foreach(_expectedTarget yunji_sdk)
  list(APPEND _expectedTargets ${_expectedTarget})
  # 检查目标是否已定义
  if(NOT TARGET ${_expectedTarget})
    list(APPEND _targetsNotDefined ${_expectedTarget})
  endif()
  if(TARGET ${_expectedTarget})
    list(APPEND _targetsDefined ${_expectedTarget})
  endif()
endforeach()

# 如果所有目标都已定义，则清理变量并返回
if("${_targetsDefined}" STREQUAL "${_expectedTargets}")
  unset(_targetsDefined)
  unset(_targetsNotDefined)
  unset(_expectedTargets)
  set(CMAKE_IMPORT_FILE_VERSION)
  cmake_policy(POP)
  return()
endif()

# 如果部分目标已定义（不一致状态），报错终止
if(NOT "${_targetsDefined}" STREQUAL "")
  message(FATAL_ERROR "Some (but not all) targets in this export set were already defined.\nTargets Defined: ${_targetsDefined}\nTargets not yet defined: ${_targetsNotDefined}\n")
endif()

# 清理临时变量
unset(_targetsDefined)
unset(_targetsNotDefined)
unset(_expectedTargets)

# 计算安装前缀（通过当前文件路径回溯）
get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)  # 注释：获取当前文件所在目录
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)           # 注释：向上回溯一级（通常是lib/cmake/yunji_sdk）
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)           # 注释：再回溯一级（通常是lib/cmake）
get_filename_component(_IMPORT_PREFIX "${_IMPORT_PREFIX}" PATH)           # 注释：再回溯一级（安装根目录如/usr/local）
if(_IMPORT_PREFIX STREQUAL "/")
  set(_IMPORT_PREFIX "")  # 注释：根目录特殊处理
endif()

# 创建导入目标 ddsc（CycloneDDS核心库）
add_library(ddsc SHARED IMPORTED GLOBAL)  # 注释：GLOBAL使目标全局可见
set_target_properties(ddsc PROPERTIES
    IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libddsc.so  # 注释：库文件绝对路径
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include;${_IMPORT_PREFIX}/include"  # 注释：头文件搜索路径
    INTERFACE_LINK_LIBRARIES "Threads::Threads"  # 注释：传递依赖（线程库）
    IMPORTED_NO_SONAME TRUE)  # 注释：标记该库无SONAME（解决非标准库链接问题）

# 创建导入目标 ddscxx（CycloneDDS C++绑定库）
add_library(ddscxx SHARED IMPORTED GLOBAL)
set_target_properties(ddscxx PROPERTIES
    IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libddscxx.so
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include;${_IMPORT_PREFIX}/include/ddscxx"  # 注释：包含两层头文件路径
    INTERFACE_LINK_LIBRARIES "Threads::Threads"
    IMPORTED_NO_SONAME TRUE)  # 注释：同上

# 创建主目标 yunji_sdk（静态库）
add_library(yunji_sdk STATIC IMPORTED GLOBAL)
set_target_properties(yunji_sdk PROPERTIES
    IMPORTED_LOCATION ${_IMPORT_PREFIX}/lib/libyunji_sdk.a  # 注释：静态库文件路径
    INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include;${_IMPORT_PREFIX}/include"  # 注释：SDK头文件路径
    INTERFACE_LINK_LIBRARIES "ddsc;ddscxx;Threads::Threads"  # 注释：显式声明依赖链
    LINKER_LANGUAGE CXX)  # 注释：强制使用C++链接器（避免C链接器错误）

# 清理临时路径变量
set(_IMPORT_PREFIX)

# 检查所有导入文件是否存在（防御性编程）
foreach(target ${_IMPORT_CHECK_TARGETS} )
  foreach(file ${_IMPORT_CHECK_FILES_FOR_${target}} )
    if(NOT EXISTS "${file}" )
      # 详细错误信息，帮助用户诊断安装问题
      message(FATAL_ERROR "The imported target \"${target}\" references the file
   \"${file}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty and contained
   \"${CMAKE_CURRENT_LIST_FILE}\"
but not all the files it references.
")
    endif()
  endforeach()
  unset(_IMPORT_CHECK_FILES_FOR_${target})
endforeach()
unset(_IMPORT_CHECK_TARGETS)

# 收尾工作：重置版本号并恢复CMake策略
set(CMAKE_IMPORT_FILE_VERSION)
cmake_policy(POP)  # 注释：恢复原始CMake策略
