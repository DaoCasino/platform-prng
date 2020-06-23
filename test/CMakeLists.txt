cmake_minimum_required(VERSION 3.5)

find_package(eosio)
find_package(Boost 1.70 REQUIRED COMPONENTS program_options)

add_subdirectory(${GAME_SDK_PATH}/tester ${CMAKE_BINARY_DIR}/tester)

configure_file(contracts.hpp.in ${CMAKE_BINARY_DIR}/contracts.hpp)

add_game_test(prng_test prng_test.cpp)
target_include_directories(prng_test PUBLIC ${CMAKE_BINARY_DIR})
target_link_libraries(prng_test ${Boost_PROGRAM_OPTIONS_LIBRARY})
