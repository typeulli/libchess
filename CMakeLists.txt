cmake_minimum_required(VERSION 4.1)
project(chesscpp)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-O3)

# ----------------------
# Shared Library (.dll)
# ----------------------
add_library(chess_lib SHARED src/libchess.cpp src/board.ipp)
target_compile_definitions(chess_lib PRIVATE BUILD_DLL)
target_link_options(chess_lib PRIVATE -static-libgcc -static-libstdc++ -static)
set_target_properties(chess_lib PROPERTIES OUTPUT_NAME "chess" PREFIX "lib" SUFFIX ".dll")

# ----------------------
# CLI Executable
# ----------------------
add_executable(chess_cli main.cpp)
target_link_libraries(chess_cli chess_lib)

# ----------------------
# Google Test
# ----------------------
enable_testing()
find_package(GTest CONFIG REQUIRED)

add_executable(chess_test test.cpp)
target_link_libraries(chess_test chess_lib GTest::gtest GTest::gtest_main)
add_test(NAME chess_test COMMAND chess_test)

# ----------------------
# Python Module (.pyd)
# ----------------------
add_library(chess_py MODULE python/libchess_py.cpp)
include_directories("C:/Program Files/Python311/include")
target_link_libraries(chess_py PRIVATE chess_lib "C:/Program Files/Python311/libs/python311.lib")
set_target_properties(chess_py PROPERTIES PREFIX "lib" OUTPUT_NAME "chess" SUFFIX ".pyd")
target_link_options(chess_py PRIVATE -static-libgcc -static-libstdc++ -static)