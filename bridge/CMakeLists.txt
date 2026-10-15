# Köprü kaynakları her oyunun istemci/sunucu hedefine ayrı ayrı derlenir, çünkü oyunun
# kendi SDK başlıklarını (hud.h, cbase.h ...) kullanırlar. Lisans: Half Life 1 SDK License.
set(LAMBDA_BRIDGE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)

set(LAMBDA_BRIDGE_CLIENT_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/client_bridge.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/client_vgui.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/engine_sdl2.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gl.cpp
	PARENT_SCOPE)

set(LAMBDA_BRIDGE_SERVER_SOURCES
	PARENT_SCOPE)
