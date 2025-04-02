add_executable(test-conf test-config.cpp)
target_include_directories(test-conf PRIVATE ../include)
target_link_libraries(test-conf jgb-core jansson)

add_executable(test-core main.cpp test-core.cpp)
target_include_directories(test-core PRIVATE ../include)
target_link_libraries(test-core jgb-core jansson)

install(FILES test_core.json DESTINATION etc/jgb/test-jgb)
