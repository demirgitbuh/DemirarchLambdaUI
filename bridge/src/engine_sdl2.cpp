#define LAMBDA_ENGINE_SDL2_NO_REDIRECT
#include "lambda_bridge/engine_sdl2.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace lambda::engine_sdl2
{
namespace
{
struct Table
{
	bool resolved = false;
	bool available = false;
	void(SDLCALL* PumpEvents)() = nullptr;
	Uint32(SDLCALL* GetRelativeMouseState)(int*, int*) = nullptr;
	int(SDLCALL* SetRelativeMouseMode)(SDL_bool) = nullptr;
	int(SDLCALL* NumJoysticks)() = nullptr;
	SDL_bool(SDLCALL* IsGameController)(int) = nullptr;
	SDL_GameController*(SDLCALL* GameControllerOpen)(int) = nullptr;
	void(SDLCALL* GameControllerClose)(SDL_GameController*) = nullptr;
	Sint16(SDLCALL* GameControllerGetAxis)(SDL_GameController*, SDL_GameControllerAxis) = nullptr;
	Uint8(SDLCALL* GameControllerGetButton)(SDL_GameController*, SDL_GameControllerButton) = nullptr;
	const char*(SDLCALL* GameControllerName)(SDL_GameController*) = nullptr;
	void(SDLCALL* JoystickUpdate)() = nullptr;
	int(SDLCALL* ShowSimpleMessageBox)(Uint32, const char*, const char*, SDL_Window*) = nullptr;
	void(SDLCALL* AddEventWatch)(SDL_EventFilter, void*) = nullptr;
	void(SDLCALL* DelEventWatch)(SDL_EventFilter, void*) = nullptr;
	void*(SDLCALL* GL_GetProcAddress)(const char*) = nullptr;
};

Table g_table;

void* FindModule()
{
#ifdef _WIN32
	// Motor SDL2.dll'yi oyun kök dizininden yüklemiş olmalı; yeni bir kopya yüklemiyoruz.
	return reinterpret_cast<void*>(GetModuleHandleW(L"SDL2.dll"));
#else
	return dlopen("libSDL2-2.0.so.0", RTLD_LAZY | RTLD_NOLOAD);
#endif
}

void* FindSymbol(void* module, const char* name)
{
#ifdef _WIN32
	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module), name));
#else
	return dlsym(module, name);
#endif
}

template <typename T>
void Bind(void* module, T& slot, const char* name)
{
	slot = reinterpret_cast<T>(FindSymbol(module, name));
}

Table& Get()
{
	if (!g_table.resolved)
	{
		g_table.resolved = true;
		if (void* module = FindModule())
		{
			Bind(module, g_table.PumpEvents, "SDL_PumpEvents");
			Bind(module, g_table.GetRelativeMouseState, "SDL_GetRelativeMouseState");
			Bind(module, g_table.SetRelativeMouseMode, "SDL_SetRelativeMouseMode");
			Bind(module, g_table.NumJoysticks, "SDL_NumJoysticks");
			Bind(module, g_table.IsGameController, "SDL_IsGameController");
			Bind(module, g_table.GameControllerOpen, "SDL_GameControllerOpen");
			Bind(module, g_table.GameControllerClose, "SDL_GameControllerClose");
			Bind(module, g_table.GameControllerGetAxis, "SDL_GameControllerGetAxis");
			Bind(module, g_table.GameControllerGetButton, "SDL_GameControllerGetButton");
			Bind(module, g_table.GameControllerName, "SDL_GameControllerName");
			Bind(module, g_table.JoystickUpdate, "SDL_JoystickUpdate");
			Bind(module, g_table.ShowSimpleMessageBox, "SDL_ShowSimpleMessageBox");
			Bind(module, g_table.AddEventWatch, "SDL_AddEventWatch");
			Bind(module, g_table.DelEventWatch, "SDL_DelEventWatch");
			Bind(module, g_table.GL_GetProcAddress, "SDL_GL_GetProcAddress");
			g_table.available = g_table.PumpEvents != nullptr;
		}
	}
	return g_table;
}
} // namespace

bool Available() { return Get().available; }

void PumpEvents()
{
	if (auto f = Get().PumpEvents)
		f();
}

Uint32 GetRelativeMouseState(int* x, int* y)
{
	if (auto f = Get().GetRelativeMouseState)
		return f(x, y);
	if (x)
		*x = 0;
	if (y)
		*y = 0;
	return 0;
}

int SetRelativeMouseMode(SDL_bool enabled)
{
	if (auto f = Get().SetRelativeMouseMode)
		return f(enabled);
	return -1;
}

int NumJoysticks()
{
	if (auto f = Get().NumJoysticks)
		return f();
	return 0;
}

SDL_bool IsGameController(int joystick_index)
{
	if (auto f = Get().IsGameController)
		return f(joystick_index);
	return SDL_FALSE;
}

SDL_GameController* GameControllerOpen(int joystick_index)
{
	if (auto f = Get().GameControllerOpen)
		return f(joystick_index);
	return nullptr;
}

void GameControllerClose(SDL_GameController* controller)
{
	if (auto f = Get().GameControllerClose; f && controller)
		f(controller);
}

Sint16 GameControllerGetAxis(SDL_GameController* controller, SDL_GameControllerAxis axis)
{
	if (auto f = Get().GameControllerGetAxis; f && controller)
		return f(controller, axis);
	return 0;
}

Uint8 GameControllerGetButton(SDL_GameController* controller, SDL_GameControllerButton button)
{
	if (auto f = Get().GameControllerGetButton; f && controller)
		return f(controller, button);
	return 0;
}

const char* GameControllerName(SDL_GameController* controller)
{
	if (auto f = Get().GameControllerName; f && controller)
		return f(controller);
	return "";
}

void JoystickUpdate()
{
	if (auto f = Get().JoystickUpdate)
		f();
}

int ShowSimpleMessageBox(Uint32 flags, const char* title, const char* message, SDL_Window* window)
{
	if (auto f = Get().ShowSimpleMessageBox)
		return f(flags, title, message, window);
	return -1;
}

void AddEventWatch(SDL_EventFilter filter, void* userdata)
{
	if (auto f = Get().AddEventWatch)
		f(filter, userdata);
}

void DelEventWatch(SDL_EventFilter filter, void* userdata)
{
	if (auto f = Get().DelEventWatch)
		f(filter, userdata);
}

void* GL_GetProcAddress(const char* name)
{
	if (auto f = Get().GL_GetProcAddress)
		return f(name);
	return nullptr;
}
} // namespace lambda::engine_sdl2
