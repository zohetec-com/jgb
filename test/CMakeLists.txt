add_executable(test-conf test-config.cpp)
target_include_directories(test-conf PRIVATE ../include)
target_link_libraries(test-conf jgb-core)

add_library(test-core SHARED test-core.cpp test-run.cpp)
target_include_directories(test-core PRIVATE ../include)
install(TARGETS test-core)
install(FILES test_core.json module.json DESTINATION etc/jgb/test-jgb)
