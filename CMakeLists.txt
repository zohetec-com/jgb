cmake_minimum_required(VERSION 3.5)
project(
    JGB
    VERSION 0.0
)
add_executable(jgb main.c install.cpp)
target_compile_options(jgb PUBLIC "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
install(FILES debug.h DESTINATION include/jgb)
