cmake_minimum_required(VERSION 3.20)
project(NULL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

add_library(null_core
    src/ledger.cpp
    src/serialization.cpp
    src/block_serialization.cpp
    src/block_validation.cpp
    src/commitment.cpp
    src/consensus.cpp
)
target_include_directories(null_core PUBLIC include)
target_compile_features(null_core PUBLIC cxx_std_20)

target_compile_options(null_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_executable(null_core_tests tests/core_tests.cpp)
target_link_libraries(null_core_tests PRIVATE null_core)
add_test(NAME null_core_tests COMMAND null_core_tests)

add_executable(null_consensus_tests tests/consensus_tests.cpp)
target_link_libraries(null_consensus_tests PRIVATE null_core)
add_test(NAME null_consensus_tests COMMAND null_consensus_tests)

add_executable(null_commitment_tests tests/commitment_tests.cpp)
target_link_libraries(null_commitment_tests PRIVATE null_core)
add_test(NAME null_commitment_tests COMMAND null_commitment_tests)

install(TARGETS null_core)
install(DIRECTORY include/ DESTINATION include)
