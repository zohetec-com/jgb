#add_executable(jgb main.cpp install.cpp)
#target_include_directories(jgb PRIVATE ../include)
#target_compile_options(jgb PUBLIC "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
#install(TARGETS jgb)

add_library(jgb-core SHARED config.cpp config_factory.cpp helper.cpp core.cpp)
target_include_directories(jgb-core PRIVATE ../include)
find_package(Boost COMPONENTS thread REQUIRED)
target_link_libraries(jgb-core Boost::thread jansson)
install(TARGETS jgb-core)
