# lambda-setup — MIT. Oyun yollarını doğrular, Blue Shift haritalarını yerelde dönüştürür,
# yalnızca modun kendi dizinlerini kaldırır.
add_library(lambda_setup_lib STATIC bsp_convert.cpp)
target_include_directories(lambda_setup_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(lambda_setup_lib PUBLIC cxx_std_17)

add_executable(lambda-setup main.cpp)
target_link_libraries(lambda-setup PRIVATE lambda_setup_lib lambda_core)
target_compile_options(lambda-setup PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>)
target_link_options(lambda-setup PRIVATE $<$<PLATFORM_ID:Linux>:-static-libstdc++ -static-libgcc>)
set_target_properties(lambda-setup PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${LAMBDA_STAGE_DIR}/lambda_tools)
foreach(_cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
	set_target_properties(lambda-setup PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${_cfg} ${LAMBDA_STAGE_DIR}/lambda_tools)
endforeach()

file(WRITE ${LAMBDA_STAGE_DIR}/lambda_tools/lambda_tools.txt
	"Demirarch Lambda UI ${LAMBDA_VERSION_STRING} tools folder. Safe to delete after uninstalling.\n")
