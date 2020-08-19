add_executable(BasicSingleTest BasicSingleTest.cpp)
target_link_libraries(BasicSingleTest MultiQueueProcessor Threads::Threads)
set_property(TARGET BasicSingleTest PROPERTY CXX_STANDARD 17)
target_compile_options(BasicSingleTest PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
     -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic>
     $<$<CXX_COMPILER_ID:MSVC>:
     /permissive /W4 /w14640 >)

add_executable(PerformanceTest PerformanceTest.cpp)
target_link_libraries(PerformanceTest MultiQueueProcessor Threads::Threads)
set_property(TARGET PerformanceTest PROPERTY CXX_STANDARD 17)
target_compile_options(BasicSingleTest PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
     -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic>
     $<$<CXX_COMPILER_ID:MSVC>:
     /permissive /W4 /w14640 >)

add_executable(DummyTest DummyTest.cpp)
target_link_libraries(DummyTest MultiQueueProcessor Threads::Threads)
set_property(TARGET DummyTest PROPERTY CXX_STANDARD 17)
target_compile_options(BasicSingleTest PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
     -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic>
     $<$<CXX_COMPILER_ID:MSVC>:
     /permissive /W4 /w14640 >)

add_test(NAME BasicSingleTest COMMAND BasicSingleTest)
add_test(NAME PerformanceTest1x4 COMMAND PerformanceTest 1 4)
add_test(NAME PerformanceTest4x4 COMMAND PerformanceTest 4 4)
add_test(NAME PerformanceTest16x2 COMMAND PerformanceTest 16 2)
add_test(NAME DummyTest COMMAND DummyTest 16 2)