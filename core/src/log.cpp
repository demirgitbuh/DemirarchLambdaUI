// SPDX-License-Identifier: MIT
#include "lambda/core/log.h"

#include "lambda/core/fileio.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <vector>


namespace lambda::log
{
namespace
{
std::mutex g_mutex;
FILE* g_file = nullptr;
std::string g_path;
Level g_min = Level::Info;
std::vector<std::pair<std::string, std::string>> g_private; // (aranan, yerine)

const char* LevelName(Level l)
{
	switch (l)
	{
	case Level::Debug: return "debug";
	case Level::Info: return "info";
	case Level::Warning: return "warn";
	case Level::Error: return "error";
	}
	return "?";
}

void CollectPrivatePrefixes()
{
	g_private.clear();
	for (const char* var : {"USERPROFILE", "HOME"})
	{
		const char* v = std::getenv(var);
		if (!v || std::string(v).size() <= 3)
			continue;
		std::string p = v;
		g_private.emplace_back(p, "~");
		for (char& c : p)
			if (c == '\\')
				c = '/';
		g_private.emplace_back(p, "~");
	}
	// Kullanıcı adları ev dizinlerinden sonra uygulanır.
	for (const char* var : {"USERNAME", "USER"})
	{
		const char* v = std::getenv(var);
		if (v && std::string(v).size() > 2)
			g_private.emplace_back(v, "<user>");
	}
}

void ReplaceAll(std::string& s, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t pos = 0;
	while ((pos = s.find(from, pos)) != std::string::npos)
	{
		s.replace(pos, from.size(), to);
		pos += to.size();
	}
}
} // namespace

std::string Sanitize(const std::string& text)
{
	std::string out = text;
	for (const auto& [from, to] : g_private)
		ReplaceAll(out, from, to);
	return out;
}

void Open(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (g_file)
		return;
	CollectPrivatePrefixes();
	fileio::MakeDirectories(directory);
	g_path = fileio::Join(directory, "lambda.log");
	const std::string previous = fileio::Join(directory, "lambda.1.log");
	if (fileio::Exists(g_path))
		fileio::Rename(g_path, previous);
	g_file = fileio::OpenFile(g_path, "w");
}

void Close()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	if (g_file)
		std::fclose(g_file);
	g_file = nullptr;
}

bool IsOpen() { return g_file != nullptr; }

void SetMinimumLevel(Level level) { g_min = level; }

const std::string& Path() { return g_path; }

void Write(Level level, const char* format, ...)
{
	if (level < g_min)
		return;
	char msg[2048];
	va_list ap;
	va_start(ap, format);
	std::vsnprintf(msg, sizeof(msg), format, ap);
	va_end(ap);

	std::lock_guard<std::mutex> lock(g_mutex);
	if (!g_file)
		return;
	const std::time_t t = std::time(nullptr);
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif
	char stamp[32];
	std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm);
	const std::string clean = Sanitize(msg);
	std::fprintf(g_file, "%s [%s] %s\n", stamp, LevelName(level), clean.c_str());
	std::fflush(g_file);
}
} // namespace lambda::log
