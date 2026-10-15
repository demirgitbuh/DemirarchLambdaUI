// Köprü kaynakları arasında paylaşılan iç durum. Yalnızca bridge/src içinden kullanılır.
#pragma once

#include "lambda_bridge/vgui2_interop.h"

namespace lambda::bridge
{
struct VGuiInterfaces
{
	vgui2::IVGui* vgui = nullptr;
	vgui2::IPanel* panel = nullptr;
	vgui2::IEngineVGui* engine_vgui = nullptr;
	vgui2::IBaseUI* baseui = nullptr;
	vgui2::IGameUI* gameui = nullptr;
	vgui2::IGameUIFuncs* gameui_funcs = nullptr;
	vgui2::IGameConsole* console = nullptr;
	vgui2::VPANEL client_parent = 0;
	vgui2::VPANEL root_panel = 0; // bizim panelimiz
	bool started = false;
	bool use_vgui1_queried = false;
	bool client_ui_active = true;
};

VGuiInterfaces& VGui();

// client_bridge.cpp: kök panelin her çiziminde çağrılır (menüde ve oyunda).
void PaintOverlay(int screen_w, int screen_h);
// client_bridge.cpp: kök panelin Think'inde çağrılır.
void ThinkOverlay();
// Kök panel fare tıklamalarını yakalamalı mı (UI açık)?
bool OverlayWantsInput();
// Arayüz alt sistemi açık mı (gamepad modu)?
bool OverlayEnabled();
} // namespace lambda::bridge
