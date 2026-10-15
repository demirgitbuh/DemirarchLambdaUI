// Demirarch Lambda UI — istemci köprüsü. Upstream istemci koduna küçük kancalarla bağlanır;
// her kanca "// LAMBDA:" yorumuyla işaretlidir. Lisans: Half Life 1 SDK License.
#pragma once

struct usercmd_s;

namespace lambda::bridge
{
// cdll_int.cpp: Initialize() içinde, gEngfuncs kopyalandıktan sonra
void OnInitialize();
// cdll_int.cpp: HUD_Init()
void OnHudInit();
// cdll_int.cpp: HUD_VidInit()
void OnVidInit();
// cdll_int.cpp: HUD_Frame() — menüdeyken de her karede çağrılır
void OnFrame(double time);
// cdll_int.cpp: HUD_Redraw() başında; true dönerse upstream HUD'u bu kare çizilmez
bool OnRedraw(float time, int intermission);
// cdll_int.cpp: HUD_PlayerMove() sonunda (su seviyesi gibi hareket durumları)
void OnPlayerMove(int waterlevel, int onground);
// input.cpp: CL_ButtonBits() sonunda OR'lanır
int ButtonBits();
// inputw32.cpp: IN_Move() sonunda (görüş açısı ve analog hareket)
void OnInMove(float frametime, usercmd_s* cmd);
// input.cpp: HUD_Key_Event() başında; 0 dönerse tuş yutulur
int OnKeyEvent(int down, int keynum, const char* binding);
// input.cpp: HUD_Shutdown()
void OnShutdown();

// Gamepad modu açık ve modun girdi katmanı çalışıyor. Klasik modda false: upstream joystick
// kodu ve özgün davranış aynen çalışır.
bool GamepadLayerActive();
} // namespace lambda::bridge

// inputw32.cpp tarafından sağlanır: motorun SDL2 üzerinden açtığı özgün joystick'i kapatır.
void IN_LambdaCloseNativeJoystick();
