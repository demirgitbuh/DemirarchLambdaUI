# lambda_core — MIT lisanslı, motordan ve SDL'den bağımsız çekirdek.
add_library(lambda_core STATIC
	src/bindings.cpp
	src/camera.cpp
	src/document.cpp
	src/fileio.cpp
	src/input.cpp
	src/mode.cpp
	src/profile_store.cpp
	src/recovery.cpp
	src/resolver.cpp
	src/settings.cpp
	src/stick.cpp)

target_include_directories(lambda_core PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_BINARY_DIR}/generated)

target_compile_features(lambda_core PUBLIC cxx_std_17)
set_target_properties(lambda_core PROPERTIES
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON)

target_compile_options(lambda_core PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8 /permissive->
	$<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wno-unused-parameter>
	# Oyun DLL'leriyle aynı FPU modu (Linux i386)
	$<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>>:-march=pentium-m -mfpmath=387>)
target_compile_definitions(lambda_core PRIVATE $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>)
