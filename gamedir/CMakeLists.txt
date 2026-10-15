# Taşınabilir paketin oyun dışı dosyaları. Orijinal oyun verisi (harita, model, ses vb.)
# kesinlikle buraya konmaz; CI paket içerik testi bunu denetler.
function(lambda_stage_moddir MOD_DIR GAME_NAME)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${MOD_DIR}/liblist.gam.in
		${LAMBDA_STAGE_DIR}/${MOD_DIR}/liblist.gam @ONLY NEWLINE_STYLE CRLF)
	# Upstream ağ delta tanımları (oyun koduyla eşleşmeli)
	configure_file(${PROJECT_SOURCE_DIR}/games/${GAME_NAME}/network/delta.lst
		${LAMBDA_STAGE_DIR}/${MOD_DIR}/delta.lst COPYONLY)
	# lambda-setup yalnızca bu işaret dosyasını içeren dizinleri kaldırır.
	file(WRITE ${LAMBDA_STAGE_DIR}/${MOD_DIR}/lambda/lambda_mod.txt
		"Demirarch Lambda UI ${LAMBDA_VERSION_STRING} (${MOD_DIR}). This folder belongs to the mod and can be removed with lambda-setup uninstall.\n")
endfunction()

lambda_stage_moddir(lambda_hl hl)
lambda_stage_moddir(lambda_op4 opfor)
lambda_stage_moddir(lambda_bs bshift)
