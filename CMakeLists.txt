cmake_minimum_required(VERSION 3.20)

if(MSVC)
	set(CMAKE_SYSTEM_VERSION 10.0)
endif()

project(DemirarchLambdaUI
	VERSION 0.1.0
	DESCRIPTION "Gamepad-first controller and interface layer for Half-Life, Opposing Force and Blue Shift"
	LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Upstream oyun kodu statik CRT ile bağlanıyor; SDL3 ve çekirdek de aynı CRT'yi kullanmalı.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SKIP_BUILD_RPATH ON)

option(LAMBDA_BUILD_GAMES "Build the Half-Life, Opposing Force and Blue Shift game libraries" ON)
option(LAMBDA_BUILD_TESTS "Build the automated test suite" ON)
option(LAMBDA_BUILD_SETUP "Build the lambda-setup tool" ON)

set(LAMBDA_STAGE_DIR "${CMAKE_BINARY_DIR}/stage" CACHE PATH "Directory where the portable mod package is assembled")

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND LAMBDA_BUILD_GAMES)
	message(FATAL_ERROR "The Half-Life engine is 32-bit. Configure with -A Win32 on Windows or the cmake/LinuxI386.cmake toolchain on Linux.")
endif()

include(cmake/LambdaVersion.cmake)
include(cmake/LambdaSDL3.cmake)
include(cmake/LambdaGame.cmake)

add_subdirectory(core)
add_subdirectory(platform)

if(LAMBDA_BUILD_GAMES)
	add_subdirectory(bridge)
	lambda_add_game(hl lambda_hl)
	lambda_add_game(opfor lambda_op4 OPFOR)
	lambda_add_game(bshift lambda_bs)
	add_subdirectory(gamedir)
endif()

if(LAMBDA_BUILD_SETUP)
	add_subdirectory(tools/setup)
endif()

if(LAMBDA_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()
