add_library(jgb-misc SHARED
    service.cpp
    read-buffer.cpp
    write-buffer.cpp)
target_include_directories(jgb-misc PRIVATE ../include)
install(TARGETS jgb-misc)
