add_executable(jgb main.c install.cpp)
target_compile_options(jgb PUBLIC "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
install(FILES debug.h DESTINATION include/jgb)

add_library(jgb-core SHARED config.cpp config_factory.cpp helper.cpp)
