cmake_minimum_required(VERSION 3.10)
project(JBLangRuntime C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

add_library(jblang_runtime STATIC
        src/runtime.c
        src/allocator_impl.c
        src/simple_allocator.c
        src/reference_count_allocator.c
        src/mark_sweep_allocator.c
        )

target_include_directories(jblang_runtime PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

install(TARGETS jblang_runtime
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        )

install(DIRECTORY include/ DESTINATION include)
