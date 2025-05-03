cmake_minimum_required(VERSION 3.24)
project(tile-animation C)

set(CMAKE_C_STANDARD 17)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fplan9-extensions")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions")
add_compile_options(-fms-extensions -Wno-switch -I.)

include_directories(c:\\Dev\\smd_dev\\devkit\\sgdk\\sgdk_current\\inc)
include_directories(c:\\Dev\\smd_dev\\devkit\\sgdk\\sgdk_current\\inc\\ext)
include_directories(c:\\Dev\\smd_dev\\devkit\\sgdk\\sgdk_current\\inc\\ext\\flash-save)
include_directories(c:\\Dev\\smd_dev\\devkit\\sgdk\\sgdk_current\\res)

include_directories(.)
include_directories(inc)
include_directories(res)
include_directories(src)


add_executable(
        src/main.c
        src/game_object.c
        src/game_object.h
        src/game.c
        src/game.h
        src/player.c
        src/globals.c
        src/explosion.c
        src/enemy.c
)