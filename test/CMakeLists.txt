add_library(test-core SHARED test-core.cpp test-config.cpp test-single-worker.cpp test-run.cpp test-log.cpp)
target_include_directories(test-core PRIVATE ../include)
install(TARGETS test-core)
install(FILES test_core.json module.json DESTINATION etc/jgb/test-jgb)

add_library(template SHARED app_template.cpp)
target_include_directories(template PRIVATE ../include)
install(TARGETS template)
