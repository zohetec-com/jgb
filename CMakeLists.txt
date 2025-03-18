cmake_minimum_required(VERSION 3.5)

project(
    JGB
    VERSION 0.0
)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(jgb main.c install.cpp)
target_compile_options(jgb PUBLIC "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
install(FILES debug.h DESTINATION include/jgb)

add_library(jgb-core SHARED config.cpp config_factory.cpp helper.cpp)

add_executable(jgb-test test-config.cpp)
target_link_libraries(jgb-test jgb-core jansson)
