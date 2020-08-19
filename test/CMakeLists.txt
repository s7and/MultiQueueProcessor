add_executable(BasicSingleTest BasicSingleTest.cpp)
target_link_libraries(BasicSingleTest MultiQueueProcessor Threads::Threads)
set_property(TARGET BasicSingleTest PROPERTY CXX_STANDARD 17)

add_executable(PerformanceTest PerformanceTest.cpp)
target_link_libraries(PerformanceTest MultiQueueProcessor Threads::Threads)
set_property(TARGET PerformanceTest PROPERTY CXX_STANDARD 17)

add_executable(SubUnsubTest SubUnsubTest.cpp)
target_link_libraries(SubUnsubTest MultiQueueProcessor Threads::Threads)
set_property(TARGET SubUnsubTest PROPERTY CXX_STANDARD 17)


add_test(NAME BasicSingleTest COMMAND BasicSingleTest)
add_test(NAME PerformanceTest1x4 COMMAND PerformanceTest 1 4)
add_test(NAME PerformanceTest4x4 COMMAND PerformanceTest 4 4)
add_test(NAME PerformanceTest16x2 COMMAND PerformanceTest 16 2)
