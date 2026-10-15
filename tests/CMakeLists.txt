add_executable(lambda_tests
	main.cpp
	test_bindings.cpp
	test_profiles.cpp
	test_stick_camera.cpp)

target_link_libraries(lambda_tests PRIVATE lambda_core)
target_compile_options(lambda_tests PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W3 /utf-8>)
target_link_options(lambda_tests PRIVATE $<$<PLATFORM_ID:Linux>:-static-libstdc++ -static-libgcc>)

if(TARGET lambda_setup_lib)
	target_sources(lambda_tests PRIVATE test_setup.cpp)
	target_link_libraries(lambda_tests PRIVATE lambda_setup_lib)
endif()

add_test(NAME lambda_tests COMMAND lambda_tests)
