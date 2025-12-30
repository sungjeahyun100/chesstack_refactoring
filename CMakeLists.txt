cmake_minimum_required(VERSION 3.16)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -pedantic)

set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/engine)
set(BOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/bot_cpp)

option(BUILD_PYBINDING "Build pybind11 Python extension" ON)

# Engine library (shared/static controlled by BUILD_SHARED_LIBS)
add_library(engine_lib
    ${ENGINE_DIR}/gameboard.cpp
    ${ENGINE_DIR}/piece_spec.cpp
    ${ENGINE_DIR}/piece_setting.cpp
    ${ENGINE_DIR}/debug.cpp

)
target_include_directories(engine_lib PUBLIC ${ENGINE_DIR})
set_target_properties(engine_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Bot library (src/bot_cpp)
add_library(bot_lib
    ${BOT_DIR}/minimax.cpp
    ${BOT_DIR}/minimax_gpt.cpp
)
# Bot depends on engine
target_include_directories(bot_lib PUBLIC ${ENGINE_DIR} ${BOT_DIR})
target_link_libraries(bot_lib PRIVATE engine_lib)
set_target_properties(bot_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

# 실행 타겟(테스트 실행기)
if(BUILD_TESTS)
    add_executable(chess_test
        test/test.cpp
    )
    target_link_libraries(chess_test PRIVATE engine_lib)
    target_include_directories(chess_test PRIVATE ${ENGINE_DIR})

    add_executable(test_bot
        test/test_bot.cpp
    )
    target_link_libraries(test_bot PRIVATE engine_lib bot_lib)
    target_include_directories(test_bot PRIVATE ${ENGINE_DIR} ${BOT_DIR})

    add_executable(test_bot_line
        test/test_bot_line.cpp
    )
    target_link_libraries(test_bot_line PRIVATE engine_lib bot_lib)
    target_include_directories(test_bot_line PRIVATE ${ENGINE_DIR} ${BOT_DIR})
endif()


if(BUILD_PYBINDING)
    execute_process(
        COMMAND python -m pybind11 --cmakedir
        OUTPUT_VARIABLE pybind11_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    find_package(pybind11 REQUIRED)
    pybind11_add_module(chess_ext
        python_bind/python_chess.cpp
    )
    target_link_libraries(chess_ext PRIVATE engine_lib bot_lib)
    target_include_directories(chess_ext PRIVATE ${ENGINE_DIR} ${BOT_DIR})
    set_target_properties(chess_ext PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
