# Linux i386 araç zinciri. Half-Life'ın Linux motoru (hl_linux) 32-bit'tir.
# Derleyici CC/CXX ortam değişkenlerinden alınır (steamrt scout SDK'da gcc-9/g++-9).
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i386)

if(DEFINED ENV{CC})
	set(CMAKE_C_COMPILER $ENV{CC})
else()
	set(CMAKE_C_COMPILER gcc)
endif()
if(DEFINED ENV{CXX})
	set(CMAKE_CXX_COMPILER $ENV{CXX})
else()
	set(CMAKE_CXX_COMPILER g++)
endif()

set(CMAKE_C_FLAGS_INIT "-m32")
# _GLIBCXX_USE_CXX11_ABI=0: upstream ile aynı; Steam Runtime libstdc++ ABI'siyle uyumlu.
set(CMAKE_CXX_FLAGS_INIT "-m32 -D_GLIBCXX_USE_CXX11_ABI=0")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-m32")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-m32")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-m32")

set(CMAKE_LIBRARY_ARCHITECTURE i386-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
