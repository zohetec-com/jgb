add_library(test-core SHARED
    test-core.cpp
    test-config.cpp
    test-single-worker.cpp
    test-setup-fail.cpp
    test-setup-fail-multiple.cpp
    test-loop-fail.cpp
    test-buffer.cpp
    test-cpp.cpp
    write-buffer-x3.cpp
    test-run.cpp
    test-no-loops.cpp
    test-log.cpp)
target_include_directories(test-core PRIVATE ../include ../misc)
#install(TARGETS test-core)
#install(FILES test_core.json module.json DESTINATION etc/jgb/test-jgb)

add_library(template SHARED app_template.cpp)
target_include_directories(template PRIVATE ../include)
#install(TARGETS template)
