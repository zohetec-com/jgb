add_library(test-core SHARED test-core.cpp test-config.cpp test-run.cpp)
target_include_directories(test-core PRIVATE ../include)
install(TARGETS test-core)
install(FILES test_core.json module.json DESTINATION etc/jgb/test-jgb)
