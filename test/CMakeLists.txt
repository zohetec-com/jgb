add_executable(jgb-test test-config.cpp)
target_include_directories(jgb-test PRIVATE ../include)
target_link_libraries(jgb-test jgb-core jansson)
