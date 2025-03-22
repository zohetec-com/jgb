cmake_minimum_required(VERSION 3.5)

project(
    JGB
    VERSION 0.0
)

add_compile_options(-Werror -Wall -Wextra)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_subdirectory(src)
add_subdirectory(test)
