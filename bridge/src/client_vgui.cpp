// Demirarch Lambda UI — IClientVGUI uygulaması ve tam ekran kök panel.
// Motor, istemciden "VClientVGUI001" arayüzünü sorgular; bu sayede harita yüklü değilken de
// (ana menüde) çizim ve güncelleme yapabiliriz. Tüm çağrılar tanılama günlüğüne yazılır,
// çünkü davranışın ilgili kısmı kapalı motor tarafından belirlenir (bkz. docs/dev/03-vgui2-interop.md).
#include "bridge_internal.h"

#include "lambda/core/log.h"

#include <cstring>

namespace lambda::bridge
{
namespace v = lambda::vgui2;

namespace
{
VGuiInterfaces g_vgui;

template <typename T>
T* FindInterface(CreateInterfaceFn* factories, int count, const char* name)
{
	for (int i = 0; i < count; ++i)
	{
		if (!factories[i])
			continue;
		int rc = 0;
		if (IBaseInterface* p = factories[i](name, &rc))
			return static_cast<T*>(p);
	}
	return nullptr;
}

class RootPanel final : public v::IClientPanel
{
public:
	v::VPANEL GetVPanel() override { return g_vgui.root_panel; }
	void Think() override { ThinkOverlay(); }
	void PerformApplySchemeSettings() override {}
	void PaintTraverse(bool, bool) override
	{
		if (!OverlayEnabled() || !g_vgui.panel)
			return;
		int w = 0, h = 0;
		g_vgui.panel->GetSize(g_vgui.root_panel, w, h);
		if (!first_paint_logged_)
		{
			first_paint_logged_ = true;
			LAMBDA_LOG_INFO("vgui2: first PaintTraverse (%dx%d)", w, h);
		}
		if (w > 0 && h > 0)
			PaintOverlay(w, h);
	}
	void Repaint() override {}
	v::VPANEL IsWithinTraverse(int, int, bool) override { return OverlayWantsInput() ? g_vgui.root_panel : 0; }
	void GetInset(int& top, int& left, int& right, int& bottom) override { top = left = right = bottom = 0; }
	void GetClipRect(int& x0, int& y0, int& x1, int& y1) override
	{
		x0 = y0 = 0;
		x1 = y1 = 0;
		if (g_vgui.panel)
			g_vgui.panel->GetSize(g_vgui.root_panel, x1, y1);
	}
	void OnChildAdded(v::VPANEL) override {}
	void OnSizeChanged(int wide, int tall) override { LAMBDA_LOG_INFO("vgui2: root panel size %dx%d", wide, tall); }
	void InternalFocusChanged(bool) override {}
	bool RequestInfo(void*) override { return false; }
	void RequestFocus(int) override {}
	bool RequestFocusPrev(v::VPANEL) override { return false; }
	bool RequestFocusNext(v::VPANEL) override { return false; }
	void OnMessage(const void*, v::VPANEL) override {}
	v::VPANEL GetCurrentKeyFocus() override { return OverlayWantsInput() ? g_vgui.root_panel : 0; }
	int GetTabPosition() override { return 0; }
	const char* GetName() override { return "LambdaRoot"; }
	const char* GetClassName() override { return "LambdaRootPanel"; }
	v::HScheme GetScheme() override { return 0; }
	bool IsProportional() override { return false; }
	bool IsAutoDeleteSet() override { return false; }
	void DeletePanel() override {}
	void* QueryInterface(int) override { return nullptr; }
	void* GetPanel() override { return nullptr; }
	const char* GetModuleName() override { return "client"; }

private:
	bool first_paint_logged_ = false;
};

RootPanel g_root;
} // namespace

VGuiInterfaces& VGui() { return g_vgui; }

class ClientVGui final : public v::IClientVGUI
{
public:
	void Initialize(CreateInterfaceFn* factories, int count) override
	{
		LAMBDA_LOG_INFO("vgui2: Initialize with %d factories", count);
		g_vgui.vgui = FindInterface<v::IVGui>(factories, count, v::kVGuiName);
		g_vgui.panel = FindInterface<v::IPanel>(factories, count, v::kPanelName);
		g_vgui.engine_vgui = FindInterface<v::IEngineVGui>(factories, count, v::kEngineVGuiName);
		g_vgui.baseui = FindInterface<v::IBaseUI>(factories, count, v::kBaseUIName);
		g_vgui.gameui = FindInterface<v::IGameUI>(factories, count, v::kGameUIName);
		g_vgui.gameui_funcs = FindInterface<v::IGameUIFuncs>(factories, count, v::kGameUIFuncsName);
		g_vgui.console = FindInterface<v::IGameConsole>(factories, count, v::kGameConsoleName);
		LAMBDA_LOG_INFO("vgui2: interfaces vgui=%d panel=%d enginevgui=%d baseui=%d gameui=%d gameuifuncs=%d console=%d",
			g_vgui.vgui != nullptr, g_vgui.panel != nullptr, g_vgui.engine_vgui != nullptr, g_vgui.baseui != nullptr,
			g_vgui.gameui != nullptr, g_vgui.gameui_funcs != nullptr, g_vgui.console != nullptr);
	}

	void Start() override
	{
		LAMBDA_LOG_INFO("vgui2: Start (parent=%u)", g_vgui.client_parent);
		if (!g_vgui.vgui || !g_vgui.panel)
		{
			LAMBDA_LOG_ERROR("vgui2: required interfaces are missing; the gamepad interface cannot be shown");
			return;
		}
		g_vgui.root_panel = g_vgui.vgui->AllocPanel();
		g_vgui.panel->Init(g_vgui.root_panel, &g_root);

		// Önce GameUI'nin de çizildiği motor kökünü dene; yoksa motorun verdiği istemci ebeveyni.
		v::VPANEL parent = g_vgui.client_parent;
		if (g_vgui.engine_vgui)
		{
			const v::VPANEL engine_root = g_vgui.engine_vgui->GetPanel(v::EngineVGuiPanel::Root);
			LAMBDA_LOG_INFO("vgui2: engine panels root=%u client=%u gameui=%u", engine_root,
				g_vgui.engine_vgui->GetPanel(v::EngineVGuiPanel::ClientDll), g_vgui.engine_vgui->GetPanel(v::EngineVGuiPanel::GameUIDll));
			if (engine_root)
				parent = engine_root;
		}
		g_vgui.panel->SetParent(g_vgui.root_panel, parent);
		int w = 0, h = 0;
		if (parent)
			g_vgui.panel->GetSize(parent, w, h);
		g_vgui.panel->SetPos(g_vgui.root_panel, 0, 0);
		g_vgui.panel->SetSize(g_vgui.root_panel, w, h);
		g_vgui.panel->SetVisible(g_vgui.root_panel, true);
		g_vgui.panel->SetMouseInputEnabled(g_vgui.root_panel, false);
		g_vgui.panel->SetKeyBoardInputEnabled(g_vgui.root_panel, false);
		g_vgui.panel->MoveToFront(g_vgui.root_panel);
		g_vgui.vgui->AddTickSignal(g_vgui.root_panel, 0);
		g_vgui.started = true;
		LAMBDA_LOG_INFO("vgui2: root panel %u attached to %u (%dx%d)", g_vgui.root_panel, parent, w, h);
	}

	void SetParent(v::VPANEL parent) override
	{
		g_vgui.client_parent = parent;
		LAMBDA_LOG_INFO("vgui2: SetParent(%u)", parent);
	}

	int UseVGUI1() override
	{
		// Upstream istemcinin VGUI1 arayüzü (skor tablosu, takım menüleri) klasik modda
		// özgün davranış için korunur.
		if (!g_vgui.use_vgui1_queried)
			LAMBDA_LOG_INFO("vgui2: UseVGUI1 queried -> 1");
		g_vgui.use_vgui1_queried = true;
		return 1;
	}

	void HideScoreBoard() override { LAMBDA_LOG_DEBUG("vgui2: HideScoreBoard"); }
	void HideAllVGUIMenu() override { LAMBDA_LOG_DEBUG("vgui2: HideAllVGUIMenu"); }

	void ActivateClientUI() override
	{
		g_vgui.client_ui_active = true;
		LAMBDA_LOG_DEBUG("vgui2: ActivateClientUI");
	}

	void HideClientUI() override
	{
		g_vgui.client_ui_active = false;
		LAMBDA_LOG_DEBUG("vgui2: HideClientUI");
	}

	void Shutdown() override { LAMBDA_LOG_INFO("vgui2: Shutdown"); }
};

namespace
{
ClientVGui g_client_vgui;
IBaseInterface* CreateClientVGui() { return &g_client_vgui; }
InterfaceReg g_client_vgui_reg(CreateClientVGui, vgui2::kClientVGuiName);
} // namespace
} // namespace lambda::bridge
