// Motorun kendi SDL2 kütüphanesine çalışma zamanında çözülen erişim.
//
// Neden: İstemci DLL'ine SDL3 statik olarak bağlanıyor. SDL2 ile SDL3 aynı adlı fakat
// farklı imzalı fonksiyonlar içeriyor (ör. SDL_GetRelativeMouseState int*/float*).
// Upstream kodun SDL2 çağrılarını bağlama zamanında çözmek, yanlış kütüphaneye bağlanma
// riski taşır. Bu başlık, upstream dosyalarında SDL2 başlıklarından SONRA eklenir ve
// kullanılan SDL2 çağrılarını motorun zaten yüklediği SDL2 modülünden alınan işlev
// işaretçilerine yönlendirir. Böylece istemci SDL2'ye hiç bağlanmaz.
#pragma once

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_messagebox.h>
#include <SDL2/SDL_mouse.h>

namespace lambda::engine_sdl2
{
// Motorun SDL2 modülü bulunduysa true. Bulunamazsa tüm çağrılar güvenli varsayılan döndürür.
bool Available();

void PumpEvents();
Uint32 GetRelativeMouseState(int* x, int* y);
int SetRelativeMouseMode(SDL_bool enabled);
int NumJoysticks();
SDL_bool IsGameController(int joystick_index);
SDL_GameController* GameControllerOpen(int joystick_index);
void GameControllerClose(SDL_GameController* controller);
Sint16 GameControllerGetAxis(SDL_GameController* controller, SDL_GameControllerAxis axis);
Uint8 GameControllerGetButton(SDL_GameController* controller, SDL_GameControllerButton button);
const char* GameControllerName(SDL_GameController* controller);
void JoystickUpdate();
int ShowSimpleMessageBox(Uint32 flags, const char* title, const char* message, SDL_Window* window);

// Lambda UI'nin klavye/fare/metin olaylarını gözlemlemesi için (motorun olay akışını değiştirmez).
void AddEventWatch(SDL_EventFilter filter, void* userdata);
void DelEventWatch(SDL_EventFilter filter, void* userdata);
// Motorun GL bağlamındaki bir OpenGL fonksiyonu (Windows'ta GL 1.1 için opengl32'ye düşer).
void* GL_GetProcAddress(const char* name);
} // namespace lambda::engine_sdl2

#ifndef LAMBDA_ENGINE_SDL2_NO_REDIRECT
#define SDL_PumpEvents lambda::engine_sdl2::PumpEvents
#define SDL_GetRelativeMouseState lambda::engine_sdl2::GetRelativeMouseState
#define SDL_SetRelativeMouseMode lambda::engine_sdl2::SetRelativeMouseMode
#define SDL_NumJoysticks lambda::engine_sdl2::NumJoysticks
#define SDL_IsGameController lambda::engine_sdl2::IsGameController
#define SDL_GameControllerOpen lambda::engine_sdl2::GameControllerOpen
#define SDL_GameControllerClose lambda::engine_sdl2::GameControllerClose
#define SDL_GameControllerGetAxis lambda::engine_sdl2::GameControllerGetAxis
#define SDL_GameControllerGetButton lambda::engine_sdl2::GameControllerGetButton
#define SDL_GameControllerName lambda::engine_sdl2::GameControllerName
#define SDL_JoystickUpdate lambda::engine_sdl2::JoystickUpdate
#define SDL_ShowSimpleMessageBox lambda::engine_sdl2::ShowSimpleMessageBox
#endif
