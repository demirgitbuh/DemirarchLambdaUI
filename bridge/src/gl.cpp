// Demirarch Lambda UI — OpenGL 1.x 2B çizim yardımcıları (motorun GL bağlamında).
#include "lambda_bridge/gl.h"

#include "lambda_bridge/engine_sdl2.h"

#ifdef _WIN32
#define LAMBDA_GLAPI __stdcall
#else
#define LAMBDA_GLAPI
#endif

namespace lambda::gl
{
namespace
{
using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLbitfield = unsigned int;
using GLfloat = float;
using GLdouble = double;
using GLubyte = unsigned char;

constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_BLEND = 0x0BE2;
constexpr GLenum GL_SRC_ALPHA = 0x0302;
constexpr GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;
constexpr GLenum GL_QUADS = 0x0007;
constexpr GLenum GL_PROJECTION = 0x1701;
constexpr GLenum GL_MODELVIEW = 0x1700;
constexpr GLenum GL_DEPTH_TEST = 0x0B71;
constexpr GLenum GL_CULL_FACE = 0x0B44;
constexpr GLenum GL_ALPHA_TEST = 0x0BC0;
constexpr GLenum GL_SCISSOR_TEST = 0x0C11;
constexpr GLenum GL_FOG = 0x0B60;
constexpr GLbitfield GL_ALL_ATTRIB_BITS = 0x000FFFFF;
constexpr GLenum GL_ALPHA = 0x1906;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;
constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
constexpr GLenum GL_LINEAR = 0x2601;
constexpr GLenum GL_TEXTURE_WRAP_S = 0x2802;
constexpr GLenum GL_TEXTURE_WRAP_T = 0x2803;
constexpr GLenum GL_CLAMP = 0x2900;
constexpr GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;
constexpr GLenum GL_TEXTURE_ENV = 0x2300;
constexpr GLenum GL_TEXTURE_ENV_MODE = 0x2200;
constexpr GLenum GL_MODULATE = 0x2100;
constexpr GLenum GL_TEXTURE_BINDING_2D = 0x8069;

struct Api
{
	void(LAMBDA_GLAPI* PushAttrib)(GLbitfield) = nullptr;
	void(LAMBDA_GLAPI* PopAttrib)() = nullptr;
	void(LAMBDA_GLAPI* MatrixMode)(GLenum) = nullptr;
	void(LAMBDA_GLAPI* PushMatrix)() = nullptr;
	void(LAMBDA_GLAPI* PopMatrix)() = nullptr;
	void(LAMBDA_GLAPI* LoadIdentity)() = nullptr;
	void(LAMBDA_GLAPI* Ortho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble) = nullptr;
	void(LAMBDA_GLAPI* Enable)(GLenum) = nullptr;
	void(LAMBDA_GLAPI* Disable)(GLenum) = nullptr;
	void(LAMBDA_GLAPI* BlendFunc)(GLenum, GLenum) = nullptr;
	void(LAMBDA_GLAPI* Begin)(GLenum) = nullptr;
	void(LAMBDA_GLAPI* End)() = nullptr;
	void(LAMBDA_GLAPI* Color4ub)(GLubyte, GLubyte, GLubyte, GLubyte) = nullptr;
	void(LAMBDA_GLAPI* Vertex2f)(GLfloat, GLfloat) = nullptr;
	void(LAMBDA_GLAPI* TexCoord2f)(GLfloat, GLfloat) = nullptr;
	void(LAMBDA_GLAPI* BindTexture)(GLenum, GLuint) = nullptr;
	void(LAMBDA_GLAPI* GenTextures)(GLsizei, GLuint*) = nullptr;
	void(LAMBDA_GLAPI* DeleteTextures)(GLsizei, const GLuint*) = nullptr;
	void(LAMBDA_GLAPI* TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) = nullptr;
	void(LAMBDA_GLAPI* TexParameteri)(GLenum, GLenum, GLint) = nullptr;
	void(LAMBDA_GLAPI* PixelStorei)(GLenum, GLint) = nullptr;
	void(LAMBDA_GLAPI* TexEnvi)(GLenum, GLenum, GLint) = nullptr;
	void(LAMBDA_GLAPI* Viewport)(GLint, GLint, GLsizei, GLsizei) = nullptr;
	void(LAMBDA_GLAPI* GetIntegerv)(GLenum, GLint*) = nullptr;
};

Api g;
bool g_loaded = false;
bool g_tried = false;
int g_depth = 0;

template <typename T>
bool Resolve(T& slot, const char* name)
{
	slot = reinterpret_cast<T>(engine_sdl2::GL_GetProcAddress(name));
	return slot != nullptr;
}
} // namespace

bool Load()
{
	if (g_tried)
		return g_loaded;
	g_tried = true;
	bool ok = true;
	ok &= Resolve(g.PushAttrib, "glPushAttrib");
	ok &= Resolve(g.PopAttrib, "glPopAttrib");
	ok &= Resolve(g.MatrixMode, "glMatrixMode");
	ok &= Resolve(g.PushMatrix, "glPushMatrix");
	ok &= Resolve(g.PopMatrix, "glPopMatrix");
	ok &= Resolve(g.LoadIdentity, "glLoadIdentity");
	ok &= Resolve(g.Ortho, "glOrtho");
	ok &= Resolve(g.Enable, "glEnable");
	ok &= Resolve(g.Disable, "glDisable");
	ok &= Resolve(g.BlendFunc, "glBlendFunc");
	ok &= Resolve(g.Begin, "glBegin");
	ok &= Resolve(g.End, "glEnd");
	ok &= Resolve(g.Color4ub, "glColor4ub");
	ok &= Resolve(g.Vertex2f, "glVertex2f");
	ok &= Resolve(g.TexCoord2f, "glTexCoord2f");
	ok &= Resolve(g.BindTexture, "glBindTexture");
	ok &= Resolve(g.GenTextures, "glGenTextures");
	ok &= Resolve(g.DeleteTextures, "glDeleteTextures");
	ok &= Resolve(g.TexImage2D, "glTexImage2D");
	ok &= Resolve(g.TexParameteri, "glTexParameteri");
	ok &= Resolve(g.PixelStorei, "glPixelStorei");
	ok &= Resolve(g.TexEnvi, "glTexEnvi");
	ok &= Resolve(g.Viewport, "glViewport");
	ok &= Resolve(g.GetIntegerv, "glGetIntegerv");
	g_loaded = ok;
	return ok;
}

bool Loaded() { return g_loaded; }

void Begin2D(int w, int h)
{
	if (!g_loaded || g_depth++ > 0)
		return;
	g.PushAttrib(GL_ALL_ATTRIB_BITS);
	g.MatrixMode(GL_PROJECTION);
	g.PushMatrix();
	g.LoadIdentity();
	g.Ortho(0.0, static_cast<double>(w), static_cast<double>(h), 0.0, -1.0, 1.0);
	g.MatrixMode(GL_MODELVIEW);
	g.PushMatrix();
	g.LoadIdentity();
	g.Viewport(0, 0, w, h);
	g.Disable(GL_DEPTH_TEST);
	g.Disable(GL_CULL_FACE);
	g.Disable(GL_ALPHA_TEST);
	g.Disable(GL_SCISSOR_TEST);
	g.Disable(GL_FOG);
	g.Disable(GL_TEXTURE_2D);
	g.Enable(GL_BLEND);
	g.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	g.TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void End2D()
{
	if (!g_loaded || g_depth == 0 || --g_depth > 0)
		return;
	g.MatrixMode(GL_MODELVIEW);
	g.PopMatrix();
	g.MatrixMode(GL_PROJECTION);
	g.PopMatrix();
	g.PopAttrib();
}

void FillRect(float x, float y, float w, float h, Color c)
{
	FillRectGradient(x, y, w, h, c, c);
}

void FillRectGradient(float x, float y, float w, float h, Color top, Color bottom)
{
	if (!g_loaded || g_depth == 0)
		return;
	g.Disable(GL_TEXTURE_2D);
	g.Begin(GL_QUADS);
	g.Color4ub(top.r, top.g, top.b, top.a);
	g.Vertex2f(x, y);
	g.Vertex2f(x + w, y);
	g.Color4ub(bottom.r, bottom.g, bottom.b, bottom.a);
	g.Vertex2f(x + w, y + h);
	g.Vertex2f(x, y + h);
	g.End();
}

void OutlineRect(float x, float y, float w, float h, float t, Color c)
{
	FillRect(x, y, w, t, c);
	FillRect(x, y + h - t, w, t, c);
	FillRect(x, y + t, t, h - 2 * t, c);
	FillRect(x + w - t, y + t, t, h - 2 * t, c);
}

unsigned int CreateAlphaTexture(int w, int h, const std::uint8_t* pixels)
{
	if (!g_loaded)
		return 0;
	GLint previous = 0;
	g.GetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
	GLuint tex = 0;
	g.GenTextures(1, &tex);
	g.BindTexture(GL_TEXTURE_2D, tex);
	g.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
	g.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	g.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	g.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	g.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	g.TexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
	g.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
	g.BindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
	return tex;
}

void DeleteTexture(unsigned int tex)
{
	if (g_loaded && tex)
		g.DeleteTextures(1, &tex);
}

void TexturedQuad(unsigned int tex, float x, float y, float w, float h, float u0, float v0, float u1, float v1, Color c)
{
	if (!g_loaded || g_depth == 0 || !tex)
		return;
	g.Enable(GL_TEXTURE_2D);
	g.BindTexture(GL_TEXTURE_2D, tex);
	g.Begin(GL_QUADS);
	g.Color4ub(c.r, c.g, c.b, c.a);
	g.TexCoord2f(u0, v0);
	g.Vertex2f(x, y);
	g.TexCoord2f(u1, v0);
	g.Vertex2f(x + w, y);
	g.TexCoord2f(u1, v1);
	g.Vertex2f(x + w, y + h);
	g.TexCoord2f(u0, v1);
	g.Vertex2f(x, y + h);
	g.End();
	g.Disable(GL_TEXTURE_2D);
}
} // namespace lambda::gl
