cmake_minimum_required(VERSION 3.5)

project(prng_contract)

add_subdirectory(${GAME_SDK_PATH}/sdk ${CMAKE_BINARY_DIR}/sdk)

add_game_contract(prng src/prng.cpp)
target_include_directories(prng PUBLIC include/)
