# lambda_platform — MIT; SDL3 (statik) üzerinden kontrolcü erişimi.
add_library(lambda_platform STATIC
	src/gamepad.cpp)

target_include_directories(lambda_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(lambda_platform PUBLIC lambda_core PRIVATE SDL3::SDL3-static)
set_target_properties(lambda_platform PROPERTIES
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON)
target_compile_options(lambda_platform PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8 /permissive->
	$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wno-unused-parameter>)
