// Motorun OpenGL bağlamında 2B çizim. Fonksiyonlar motorun SDL2'si üzerinden
// (SDL_GL_GetProcAddress) çalışma zamanında çözülür; istemci OpenGL kütüphanesine bağlanmaz.
// Tüm çizimler durum kaydedilip geri yüklenerek yapılır, motorun GL durumu bozulmaz.
#pragma once

#include <cstdint>

namespace lambda::gl
{
struct Color
{
	std::uint8_t r = 255, g = 255, b = 255, a = 255;
};

bool Load();
bool Loaded();

// Ekran koordinatlarında (sol üst 0,0) 2B çizim bloğu.
void Begin2D(int screen_w, int screen_h);
void End2D();
void FillRect(float x, float y, float w, float h, Color c);
void FillRectGradient(float x, float y, float w, float h, Color top, Color bottom);
void OutlineRect(float x, float y, float w, float h, float thickness, Color c);

// Tek kanallı (alfa) doku; yazı tipi atlası için.
unsigned int CreateAlphaTexture(int w, int h, const std::uint8_t* pixels);
void DeleteTexture(unsigned int tex);
void TexturedQuad(unsigned int tex, float x, float y, float w, float h, float u0, float v0, float u1, float v1, Color c);
} // namespace lambda::gl
