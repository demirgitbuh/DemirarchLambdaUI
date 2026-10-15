# Üç oyunun (hl, opfor, bshift) istemci ve sunucu DLL'lerini tek derleme ağacında üretir.
# Hedef adları çakışmasın diye lambda_<oyun>_client / lambda_<oyun>_server kullanılır;
# çıktı adları motorun beklediği client / hl olarak kalır.

# Upstream halflife-updated CMakeLists.txt içindeki set_common_properties ile aynı ayarlar.
function(lambda_game_common_properties TARGET_NAME)
	set_target_properties(${TARGET_NAME} PROPERTIES
		PREFIX ""
		CXX_VISIBILITY_PRESET hidden
		VISIBILITY_INLINES_HIDDEN ON
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

	target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)

	target_compile_definitions(${TARGET_NAME} PRIVATE
		_CRT_SECURE_NO_WARNINGS
		$<$<CONFIG:Debug>:_DEBUG>
		CLIENT_WEAPONS
		$<$<PLATFORM_ID:Linux>:POSIX _POSIX LINUX _LINUX GNUC>
		$<$<PLATFORM_ID:Windows>:WIN32 _WINDOWS>
		UNIFIED_SDK_CONFIG="$<CONFIG>")

	target_link_libraries(${TARGET_NAME} PRIVATE ${CMAKE_DL_LIBS})

	target_compile_options(${TARGET_NAME} PRIVATE
		# Win32/Linux kayan nokta hassasiyeti eşleşsin diye 387 FPU (upstream ile aynı)
		$<$<CXX_COMPILER_ID:Clang,GNU>:-fpermissive -fno-strict-aliasing -Wno-invalid-offsetof -march=pentium-m -mfpmath=387>
		$<$<CXX_COMPILER_ID:GNU>:-flifetime-dse=1 -fno-gnu-unique>
		$<$<CXX_COMPILER_ID:MSVC>:/W3 /MP /utf-8 /wd4244 /wd4305 /wd4514 /wd4100 /wd26495 /wd26451 /wd26812>)

	target_link_options(${TARGET_NAME} PRIVATE
		$<$<PLATFORM_ID:Linux>:-static-libstdc++ -static-libgcc -Wl,--no-undefined -Wl,--exclude-libs,ALL>)
endfunction()

# Motorla birlikte gelen VGUI1 kütüphanesi. Linux'ta vgui.so hl_linux'un yanında aranır; bu
# yüzden yol bileşeni olmadan bağlanmalıdır (upstream notu).
# SDL2'ye bağlanılmaz: istemci motorun SDL2'sine çalışma zamanında erişir
# (bridge/include/lambda_bridge/engine_sdl2.h), çünkü SDL3 statik olarak bağlanıyor.
function(lambda_import_engine_libs GAME_DIR GAME_NAME)
	set(_vgui lambda_${GAME_NAME}_vgui)
	set(_sdl2 lambda_${GAME_NAME}_sdl2_headers)
	add_library(${_sdl2} INTERFACE)
	target_include_directories(${_sdl2} INTERFACE ${GAME_DIR}/external)

	if(WIN32)
		add_library(${_vgui} SHARED IMPORTED GLOBAL)
		set_target_properties(${_vgui} PROPERTIES IMPORTED_IMPLIB "${GAME_DIR}/utils/vgui/lib/win32_vc16/vgui.lib")
		target_include_directories(${_vgui} INTERFACE ${GAME_DIR}/utils/vgui/include)
	else()
		# vgui.so'nun SONAME'i yok. Tam yol verilirse DT_NEEDED'a derleme makinesinin yolu yazılır;
		# "-l:vgui.so" ile yalnızca dosya adı kaydedilir ve motor onu hl_linux'un yanında bulur.
		set(_libdir ${CMAKE_BINARY_DIR}/enginelibs/${GAME_NAME})
		file(MAKE_DIRECTORY ${_libdir})
		configure_file(${GAME_DIR}/linux/vgui.so ${_libdir}/vgui.so COPYONLY)
		add_library(${_vgui} INTERFACE)
		target_include_directories(${_vgui} INTERFACE ${GAME_DIR}/utils/vgui/include)
		target_link_directories(${_vgui} INTERFACE ${_libdir})
		target_link_libraries(${_vgui} INTERFACE "-l:vgui.so")
	endif()
endfunction()

# lambda_add_game(<ad> <mod dizini> [OPFOR])
function(lambda_add_game GAME_NAME MOD_DIR)
	cmake_parse_arguments(ARG "OPFOR" "" "" ${ARGN})
	set(GAME_DIR ${PROJECT_SOURCE_DIR}/games/${GAME_NAME})
	include(${GAME_DIR}/sources.cmake)

	set(_game_defs)
	if(ARG_OPFOR)
		list(APPEND _game_defs OPFOR)
	endif()

	lambda_import_engine_libs(${GAME_DIR} ${GAME_NAME})

	# --- İstemci (cl_dlls/client) ---
	set(_client lambda_${GAME_NAME}_client)
	add_library(${_client} SHARED)
	list(TRANSFORM GAME_CLIENT_SOURCES PREPEND ${GAME_DIR}/)
	target_sources(${_client} PRIVATE ${GAME_CLIENT_SOURCES} ${LAMBDA_BRIDGE_CLIENT_SOURCES})
	lambda_game_common_properties(${_client})
	target_include_directories(${_client} PRIVATE
		${GAME_DIR}/cl_dll
		${GAME_DIR}/cl_dll/hl
		${GAME_DIR}/cl_dll/particleman
		${GAME_DIR}/dlls
		${GAME_DIR}/common
		${GAME_DIR}/game_shared
		${GAME_DIR}/public
		${GAME_DIR}/pm_shared
		${GAME_DIR}/engine
		${GAME_DIR}/utils/vgui/include
		${LAMBDA_BRIDGE_INCLUDE_DIR})
	target_compile_definitions(${_client} PRIVATE CLIENT_DLL HL_DLL ${_game_defs}
		LAMBDA_GAME_ID="${GAME_NAME}" LAMBDA_MOD_DIR="${MOD_DIR}")
	target_link_libraries(${_client} PRIVATE
		lambda_${GAME_NAME}_vgui
		lambda_${GAME_NAME}_sdl2_headers
		lambda_platform
		lambda_core
		$<$<PLATFORM_ID:Windows>:wsock32>)
	set_target_properties(${_client} PROPERTIES
		OUTPUT_NAME client
		INSTALL_RPATH "$ORIGIN"
		LIBRARY_OUTPUT_DIRECTORY ${LAMBDA_STAGE_DIR}/${MOD_DIR}/cl_dlls
		RUNTIME_OUTPUT_DIRECTORY ${LAMBDA_STAGE_DIR}/${MOD_DIR}/cl_dlls
		ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/implib/${GAME_NAME}/client
		PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pdb/${GAME_NAME}/client)
	foreach(_cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
		set_target_properties(${_client} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY_${_cfg} ${LAMBDA_STAGE_DIR}/${MOD_DIR}/cl_dlls
			RUNTIME_OUTPUT_DIRECTORY_${_cfg} ${LAMBDA_STAGE_DIR}/${MOD_DIR}/cl_dlls
			# İçe aktarma kitaplıkları (.lib/.exp) oyun başına ayrı; aynı adlı hedefler çakışmasın.
			ARCHIVE_OUTPUT_DIRECTORY_${_cfg} ${CMAKE_BINARY_DIR}/implib/${GAME_NAME}/client)
	endforeach()

	# --- Sunucu (dlls/hl) ---
	set(_server lambda_${GAME_NAME}_server)
	add_library(${_server} SHARED)
	list(TRANSFORM GAME_SERVER_SOURCES PREPEND ${GAME_DIR}/)
	target_sources(${_server} PRIVATE ${GAME_SERVER_SOURCES} ${LAMBDA_BRIDGE_SERVER_SOURCES}
		$<$<PLATFORM_ID:Windows>:${GAME_DIR}/dlls/hl.def>)
	lambda_game_common_properties(${_server})
	target_include_directories(${_server} PRIVATE
		${GAME_DIR}/dlls
		${GAME_DIR}/common
		${GAME_DIR}/engine
		${GAME_DIR}/game_shared
		${GAME_DIR}/pm_shared
		${GAME_DIR}/public
		${LAMBDA_BRIDGE_INCLUDE_DIR})
	target_compile_definitions(${_server} PRIVATE VALVE_DLL QUIVER VOXEL QUAKE2 ${_game_defs}
		LAMBDA_GAME_ID="${GAME_NAME}" LAMBDA_MOD_DIR="${MOD_DIR}")
	target_link_libraries(${_server} PRIVATE lambda_core)
	set_target_properties(${_server} PROPERTIES
		OUTPUT_NAME hl
		LIBRARY_OUTPUT_DIRECTORY ${LAMBDA_STAGE_DIR}/${MOD_DIR}/dlls
		RUNTIME_OUTPUT_DIRECTORY ${LAMBDA_STAGE_DIR}/${MOD_DIR}/dlls
		ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/implib/${GAME_NAME}/server
		PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pdb/${GAME_NAME}/server)
	foreach(_cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
		set_target_properties(${_server} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY_${_cfg} ${LAMBDA_STAGE_DIR}/${MOD_DIR}/dlls
			RUNTIME_OUTPUT_DIRECTORY_${_cfg} ${LAMBDA_STAGE_DIR}/${MOD_DIR}/dlls
			ARCHIVE_OUTPUT_DIRECTORY_${_cfg} ${CMAKE_BINARY_DIR}/implib/${GAME_NAME}/server)
	endforeach()
endfunction()
