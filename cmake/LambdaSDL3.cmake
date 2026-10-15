# SDL3 yalnızca kontrolcü için gereken alt sistemlerle statik derlenir.
# - Statik: mod dizini dışına DLL koymadan yüklenebilsin diye (motor kök dizinine dosya eklemiyoruz).
# - Video/ses/render kapalı: pencere ve ses motorun SDL2'sinde kalır, iki kütüphane çakışmaz.
# - Linux'ta bağımlılıklar (libudev vb.) dlopen ile yüklenir; sembolleri istemci .so'sundan
#   dışarı sızmaz (--exclude-libs,ALL, bkz. LambdaGame.cmake). CI bunu nm ile doğrular.

set(LAMBDA_SDL3_DIR "${PROJECT_SOURCE_DIR}/third_party/SDL")
if(NOT EXISTS "${LAMBDA_SDL3_DIR}/CMakeLists.txt")
	message(FATAL_ERROR "SDL3 submodule is missing. Run: git submodule update --init --recursive")
endif()

macro(lambda_sdl_option NAME VALUE)
	set(${NAME} ${VALUE} CACHE BOOL "" FORCE)
endmacro()

lambda_sdl_option(SDL_SHARED OFF)
lambda_sdl_option(SDL_STATIC ON)
lambda_sdl_option(SDL_TEST_LIBRARY OFF)
lambda_sdl_option(SDL_TESTS OFF)
lambda_sdl_option(SDL_EXAMPLES OFF)
lambda_sdl_option(SDL_INSTALL OFF)
lambda_sdl_option(SDL_UNINSTALL OFF)

# Windows'ta DirectInput arka ucu (genel HID/DInput kontrolcüleri için) SDL_DIRECTX'e, o da
# SDL_AUDIO ya da SDL_VIDEO'ya bağlı. Ses alt sistemi yalnızca derlenir, hiç başlatılmaz.
if(WIN32)
	lambda_sdl_option(SDL_AUDIO ON)
	lambda_sdl_option(SDL_DIRECTX ON)
else()
	lambda_sdl_option(SDL_AUDIO OFF)
endif()
lambda_sdl_option(SDL_VIDEO OFF)
lambda_sdl_option(SDL_GPU OFF)
lambda_sdl_option(SDL_RENDER OFF)
lambda_sdl_option(SDL_CAMERA OFF)
lambda_sdl_option(SDL_DIALOG OFF)
lambda_sdl_option(SDL_TRAY OFF)

lambda_sdl_option(SDL_JOYSTICK ON)
lambda_sdl_option(SDL_HAPTIC ON)
lambda_sdl_option(SDL_HIDAPI ON)
lambda_sdl_option(SDL_POWER ON)
lambda_sdl_option(SDL_SENSOR ON)
lambda_sdl_option(SDL_VIRTUAL_JOYSTICK ON)

lambda_sdl_option(SDL_HIDAPI_LIBUSB OFF)
lambda_sdl_option(SDL_LIBUDEV ON)
lambda_sdl_option(SDL_DEPS_SHARED ON)
lambda_sdl_option(SDL_DBUS OFF)
lambda_sdl_option(SDL_IBUS OFF)
lambda_sdl_option(SDL_PIPEWIRE OFF)
lambda_sdl_option(SDL_SYSTEM_ICONV OFF)
lambda_sdl_option(SDL_RPATH OFF)
lambda_sdl_option(SDL_CLANG_TIDY OFF)
# Pencere sistemi (X11/Wayland) olmadan derleme bilinçli: pencere motorun SDL2'sinde.
lambda_sdl_option(SDL_UNIX_CONSOLE_BUILD ON)

add_subdirectory("${LAMBDA_SDL3_DIR}" "${CMAKE_BINARY_DIR}/third_party/SDL" EXCLUDE_FROM_ALL)
