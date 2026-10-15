// Demirarch Lambda UI — GoldSrc VGUI2 birlikte çalışabilirlik bildirimleri.
//
// Bu dosya Source SDK kodu içermez. Motorun (hw.dll / hw.so), vgui2 ve GameUI kütüphanelerinin
// dışa açtığı arayüzlerin yöntem SIRASI (vtable düzeni) birlikte çalışabilirlik için gereklidir;
// burada yalnızca kullandığımız yöntemlere kadar olan önekler kendi bildirimlerimizle tanımlanır.
// Sıra bilgisi topluluğun GoldSrc tersine mühendislik notlarından doğrulanmıştır
// (bkz. docs/dev/03-vgui2-interop.md). Bir yöntem eklenip çıkarılırsa sonraki tüm çağrılar
// yanlış yere gider; bu yüzden önekler eksiksiz ve sıralı tutulmalıdır.
#pragma once

#include "interface.h" // IBaseInterface, CreateInterfaceFn (HL SDK)

// windows.h GetClassName'i makroya çevirir; vtable yuvası değişmese de adlar tutarlı kalsın.
#ifdef GetClassName
#undef GetClassName
#endif

struct cl_enginefuncs_s;

namespace lambda::vgui2
{
using VPANEL = unsigned int;
using HScheme = unsigned long;

constexpr const char* kClientVGuiName = "VClientVGUI001";
constexpr const char* kEngineVGuiName = "VEngineVGui001";
constexpr const char* kBaseUIName = "BaseUI001";
constexpr const char* kGameUIName = "GameUI007";
constexpr const char* kGameUIFuncsName = "VENGINE_GAMEUIFUNCS_VERSION001";
constexpr const char* kGameConsoleName = "GameConsole003";
constexpr const char* kVGuiName = "VGUI_ivgui006";
constexpr const char* kPanelName = "VGUI_Panel007";

// Motorun istemciden sorguladığı arayüz (istemci uygular).
class IClientVGUI : public IBaseInterface
{
public:
	virtual void Initialize(CreateInterfaceFn* factories, int count) = 0;
	virtual void Start() = 0;
	virtual void SetParent(VPANEL parent) = 0;
	virtual int UseVGUI1() = 0;
	virtual void HideScoreBoard() = 0;
	virtual void HideAllVGUIMenu() = 0;
	virtual void ActivateClientUI() = 0;
	virtual void HideClientUI() = 0;
	virtual void Shutdown() = 0;
};

enum class EngineVGuiPanel : int
{
	Root = 0,
	ClientDll = 1,
	GameUIDll = 2,
};

class IEngineVGui : public IBaseInterface
{
public:
	virtual VPANEL GetPanel(EngineVGuiPanel type) = 0;
};

class IBaseUI : public IBaseInterface
{
public:
	virtual void Initialize(CreateInterfaceFn* factories, int count) = 0;
	virtual void Start(cl_enginefuncs_s* engfuncs, int version) = 0;
	virtual void Shutdown() = 0;
	virtual int Key_Event(int down, int keynum, const char* binding) = 0;
	virtual void CallEngineSurfaceAppHandler(void* event, void* userdata) = 0;
	virtual void Paint(int x, int y, int right, int bottom) = 0;
	virtual void HideGameUI() = 0;
	virtual void ActivateGameUI() = 0;
	virtual void HideConsole() = 0;
	virtual void ShowConsole() = 0;
};

class IGameUI : public IBaseInterface
{
public:
	virtual void Initialize(CreateInterfaceFn* factories, int count) = 0;
	virtual void Start(cl_enginefuncs_s* engfuncs, int version, void* system) = 0;
	virtual void Shutdown() = 0;
	virtual int ActivateGameUI() = 0;
	virtual int ActivateDemoUI() = 0;
	virtual int HasExclusiveInput() = 0;
	virtual void RunFrame() = 0;
	virtual void ConnectToServer(const char* game, int ip, int port) = 0;
	virtual void DisconnectFromServer() = 0;
	virtual void HideGameUI() = 0;
	virtual bool IsGameUIActive() = 0;
	virtual void LoadingStarted(const char* resource_type, const char* resource_name) = 0;
	virtual void LoadingFinished(const char* resource_type, const char* resource_name) = 0;
	virtual void StartProgressBar(const char* progress_type, int steps) = 0;
	virtual int ContinueProgressBar(int point, float fraction) = 0;
	virtual void StopProgressBar(bool error, const char* reason, const char* extended) = 0;
	virtual int SetProgressBarStatusText(const char* text) = 0;
	virtual void SetSecondaryProgressBar(float progress) = 0;
	virtual void SetSecondaryProgressBarText(const char* text) = 0;
	virtual void ValidateCDKey(bool, bool) = 0;
	virtual void OnDisconnectFromServer(int steam_login_failure, const char* username) = 0;
	virtual void SetIsRunningOnSteamDeck(bool on_deck) = 0;
	virtual bool IsRunningOnSteamDeck() const = 0;
};

struct VideoMode
{
	int width;
	int height;
	int bpp;
};

class IGameUIFuncs : public IBaseInterface
{
public:
	virtual bool IsKeyDown(const char* key_name, bool& is_down) = 0;
	virtual const char* Key_NameForKey(int key) = 0;
	virtual const char* Key_BindingForKey(int key) = 0;
	virtual int GetVGUI2KeyCodeForBind(const char* bind) = 0;
	virtual void GetVideoModes(VideoMode** modes, int* count) = 0;
	virtual void GetCurrentVideoMode(int* width, int* height, int* bpp) = 0;
	virtual void GetCurrentRenderer(char* name, int name_len, int* windowed, int* hd_models, int* addons_folder, int* vid_level) = 0;
	virtual bool IsConnectedToVACSecureServer() = 0;
	virtual int Key_KeyStringToKeyNum(const char* key_name) = 0;
};

class IGameConsole : public IBaseInterface
{
public:
	virtual void Activate() = 0;
	virtual void Initialize() = 0;
	virtual void Hide() = 0;
	virtual void Clear() = 0;
	virtual bool IsConsoleVisible() = 0;
	virtual void Printf(const char* format, ...) = 0;
	virtual void DPrintf(const char* format, ...) = 0;
	virtual void SetParent(int parent) = 0;
};

// vgui2'nin çağırdığı istemci paneli arayüzü (tamamı uygulanmalı; vgui2 her yuvayı çağırabilir).
class IClientPanel
{
public:
	virtual VPANEL GetVPanel() = 0;
	virtual void Think() = 0;
	virtual void PerformApplySchemeSettings() = 0;
	virtual void PaintTraverse(bool force_repaint, bool allow_force) = 0;
	virtual void Repaint() = 0;
	virtual VPANEL IsWithinTraverse(int x, int y, bool traverse_popups) = 0;
	virtual void GetInset(int& top, int& left, int& right, int& bottom) = 0;
	virtual void GetClipRect(int& x0, int& y0, int& x1, int& y1) = 0;
	virtual void OnChildAdded(VPANEL child) = 0;
	virtual void OnSizeChanged(int wide, int tall) = 0;
	virtual void InternalFocusChanged(bool lost) = 0;
	virtual bool RequestInfo(void* output_data) = 0;
	virtual void RequestFocus(int direction) = 0;
	virtual bool RequestFocusPrev(VPANEL existing) = 0;
	virtual bool RequestFocusNext(VPANEL existing) = 0;
	virtual void OnMessage(const void* params, VPANEL from) = 0;
	virtual VPANEL GetCurrentKeyFocus() = 0;
	virtual int GetTabPosition() = 0;
	virtual const char* GetName() = 0;
	virtual const char* GetClassName() = 0;
	virtual HScheme GetScheme() = 0;
	virtual bool IsProportional() = 0;
	virtual bool IsAutoDeleteSet() = 0;
	virtual void DeletePanel() = 0;
	virtual void* QueryInterface(int id) = 0;
	virtual void* GetPanel() = 0;
	virtual const char* GetModuleName() = 0;
};

// vgui2.dll — yalnızca kullanılan yöntemlere kadar önek.
class IVGui : public IBaseInterface
{
public:
	virtual bool Init(CreateInterfaceFn* factories, int count) = 0;
	virtual void Shutdown() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual bool IsRunning() = 0;
	virtual void RunFrame() = 0;
	virtual void ShutdownMessage(unsigned int id) = 0;
	virtual VPANEL AllocPanel() = 0;
	virtual void FreePanel(VPANEL panel) = 0;
	virtual void DPrintf(const char* format, ...) = 0;
	virtual void DPrintf2(const char* format, ...) = 0;
	virtual void SpewAllActivePanelNames() = 0;
	virtual unsigned long PanelToHandle(VPANEL panel) = 0;
	virtual VPANEL HandleToPanel(unsigned long handle) = 0;
	virtual void MarkPanelForDeletion(VPANEL panel) = 0;
	virtual void AddTickSignal(VPANEL panel, int interval_ms) = 0;
	virtual void RemoveTickSignal(VPANEL panel) = 0;
};

// vgui2.dll — yalnızca kullanılan yöntemlere kadar önek.
class IPanel : public IBaseInterface
{
public:
	virtual void Init(VPANEL panel, IClientPanel* client) = 0;
	virtual void SetPos(VPANEL panel, int x, int y) = 0;
	virtual void GetPos(VPANEL panel, int& x, int& y) = 0;
	virtual void SetSize(VPANEL panel, int wide, int tall) = 0;
	virtual void GetSize(VPANEL panel, int& wide, int& tall) = 0;
	virtual void SetMinimumSize(VPANEL panel, int wide, int tall) = 0;
	virtual void GetMinimumSize(VPANEL panel, int& wide, int& tall) = 0;
	virtual void SetZPos(VPANEL panel, int z) = 0;
	virtual int GetZPos(VPANEL panel) = 0;
	virtual void GetAbsPos(VPANEL panel, int& x, int& y) = 0;
	virtual void GetClipRect(VPANEL panel, int& x0, int& y0, int& x1, int& y1) = 0;
	virtual void SetInset(VPANEL panel, int left, int top, int right, int bottom) = 0;
	virtual void GetInset(VPANEL panel, int& left, int& top, int& right, int& bottom) = 0;
	virtual void SetVisible(VPANEL panel, bool state) = 0;
	virtual bool IsVisible(VPANEL panel) = 0;
	virtual void SetParent(VPANEL panel, VPANEL parent) = 0;
	virtual int GetChildCount(VPANEL panel) = 0;
	virtual VPANEL GetChild(VPANEL panel, int index) = 0;
	virtual VPANEL GetParent(VPANEL panel) = 0;
	virtual void MoveToFront(VPANEL panel) = 0;
	virtual void MoveToBack(VPANEL panel) = 0;
	virtual bool HasParent(VPANEL panel, VPANEL potential_parent) = 0;
	virtual bool IsPopup(VPANEL panel) = 0;
	virtual void SetPopup(VPANEL panel, bool state) = 0;
	virtual bool Render_GetPopupVisible(VPANEL panel) = 0;
	virtual void Render_SetPopupVisible(VPANEL panel, bool state) = 0;
	virtual HScheme GetScheme(VPANEL panel) = 0;
	virtual bool IsProportional(VPANEL panel) = 0;
	virtual bool IsAutoDeleteSet(VPANEL panel) = 0;
	virtual void DeletePanel(VPANEL panel) = 0;
	virtual void SetKeyBoardInputEnabled(VPANEL panel, bool state) = 0;
	virtual void SetMouseInputEnabled(VPANEL panel, bool state) = 0;
	virtual bool IsKeyBoardInputEnabled(VPANEL panel) = 0;
	virtual bool IsMouseInputEnabled(VPANEL panel) = 0;
};
} // namespace lambda::vgui2
