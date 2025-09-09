add_library(jgb-core SHARED
    log.cpp
    config.cpp
    schema.cpp
    config_factory.cpp
    helper.cpp
    core.cpp
    buffer.cpp
    module.cpp)
target_include_directories(jgb-core PRIVATE ../include)
find_package(Boost COMPONENTS thread chrono filesystem REQUIRED)
target_link_libraries(jgb-core ${Boost_THREAD_LIBRARY} ${Boost_CHRONO_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} jansson pcre2-8 dl)
install(TARGETS jgb-core)

add_executable(jgb main.cpp)
target_include_directories(jgb PRIVATE ../include)
target_link_libraries(jgb jgb-core)
install(TARGETS jgb)
