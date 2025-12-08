cmake_minimum_required(VERSION 3.10)
project(WebServer_from_zero)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -O2")

set(CMAKE_EXPORT_COMPILECOMMANDS ON)

# 输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 包含路径
include_directories(${PROJECT_SOURCE_DIR}/code)

# --- 阶段性测试: Buffer 模块 ---
add_executable(test_buffer 
    test/test_buffer.cpp 
    code/buffer/buffer.cpp
)

# --- 最终目标 (暂时注释，待后续阶段开启) ---
# file(GLOB_RECURSE SRC_FILES code/*.cpp)
# add_executable(server ${SRC_FILES})
# target_link_libraries(server mysqlclient)

