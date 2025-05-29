cmake_minimum_required(VERSION 3.16)
project(measure_loader_speed LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------
# 1. 告诉 CMake llama.cpp 的源码放在哪里
#    默认 ../../llama.cpp ，也可以在命令行 -DLLAMA_SRC_ROOT=/whatever
# ------------------------------------------------------------
if(NOT DEFINED LLAMA_SRC_ROOT)
    set(LLAMA_SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../llama.cpp")
endif()
message(STATUS "llama.cpp source at: ${LLAMA_SRC_ROOT}")

# ------------------------------------------------------------
# 2. 把 llama.cpp 当子项目拉进来编译，放到 build/llama-build
#    EXCLUDE_FROM_ALL=ON -> 不会把它的 examples/tests 一起编
# ------------------------------------------------------------
add_subdirectory(
    ${LLAMA_SRC_ROOT}
    ${CMAKE_BINARY_DIR}/llama-build
    EXCLUDE_FROM_ALL
)

# ------------------------------------------------------------
# 3. 本项目源文件
# ------------------------------------------------------------
add_executable(measure_loader_speed
    main.cpp
    loader/load_model.cpp
)

# ------------------------------------------------------------
# 4. 头文件搜索路径
#    include/ 里是公开 API，src/ 里有模型加载实现
# ------------------------------------------------------------
target_include_directories(measure_loader_speed
    PRIVATE
        ${LLAMA_SRC_ROOT}/include
        ${LLAMA_SRC_ROOT}/src
)

# ------------------------------------------------------------
# 5. 链接刚刚编出来的 static / shared libllama
# ------------------------------------------------------------
target_link_libraries(measure_loader_speed
    PRIVATE llama           # 由 llama.cpp 的 CMakeLists 生成
)

# ------------------------------------------------------------
# 6. 输出目录：和主 CMake 保持一致，直接放 build/bin
# ------------------------------------------------------------
set_target_properties(measure_loader_speed
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
