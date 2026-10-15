# Sürüm bilgisi tek kaynaktan (project VERSION) üretilir.
find_package(Git QUIET)
set(LAMBDA_GIT_REVISION "unknown")
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --short=10 HEAD
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		OUTPUT_VARIABLE LAMBDA_GIT_REVISION
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET)
	if(NOT LAMBDA_GIT_REVISION)
		set(LAMBDA_GIT_REVISION "unknown")
	endif()
endif()

set(LAMBDA_VERSION_STRING "${PROJECT_VERSION}")
configure_file(${PROJECT_SOURCE_DIR}/cmake/lambda_version.h.in
	${CMAKE_BINARY_DIR}/generated/lambda/version.h @ONLY)
