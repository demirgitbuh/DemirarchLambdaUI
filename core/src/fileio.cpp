// SPDX-License-Identifier: MIT
#include "lambda/core/fileio.h"

#include <cstdio>
#include <ctime>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace lambda::fileio
{
#ifdef _WIN32
namespace
{
std::wstring Wide(const std::string& s)
{
	if (s.empty())
		return {};
	const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
	std::wstring w(static_cast<size_t>(n), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
	return w;
}

std::string Narrow(const std::wstring& w)
{
	if (w.empty())
		return {};
	const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
	std::string s(static_cast<size_t>(n), '\0');
	WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
	return s;
}

FILE* OpenFile(const std::string& path, const wchar_t* mode) { return _wfopen(Wide(path).c_str(), mode); }
} // namespace
#else
namespace
{
FILE* OpenFile(const std::string& path, const char* mode) { return std::fopen(path.c_str(), mode); }
} // namespace
#endif

std::optional<std::string> ReadFile(const std::string& path, size_t max_bytes)
{
#ifdef _WIN32
	FILE* f = OpenFile(path, L"rb");
#else
	FILE* f = OpenFile(path, "rb");
#endif
	if (!f)
		return std::nullopt;
	std::string data;
	char buf[8192];
	size_t n;
	while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
	{
		data.append(buf, n);
		if (data.size() > max_bytes)
		{
			std::fclose(f);
			return std::nullopt;
		}
	}
	const bool err = std::ferror(f) != 0;
	std::fclose(f);
	if (err)
		return std::nullopt;
	return data;
}

bool WriteFileAtomic(const std::string& path, const std::string& contents)
{
	const std::string tmp = path + ".tmp";
#ifdef _WIN32
	FILE* f = OpenFile(tmp, L"wb");
#else
	FILE* f = OpenFile(tmp, "wb");
#endif
	if (!f)
		return false;
	bool ok = std::fwrite(contents.data(), 1, contents.size(), f) == contents.size();
	ok = ok && std::fflush(f) == 0;
#ifdef _WIN32
	ok = ok && _commit(_fileno(f)) == 0;
#else
	ok = ok && fsync(fileno(f)) == 0;
#endif
	ok = (std::fclose(f) == 0) && ok;
	if (!ok)
	{
		RemoveFile(tmp);
		return false;
	}
	if (!Rename(tmp, path))
	{
		RemoveFile(tmp);
		return false;
	}
	return true;
}

bool Exists(const std::string& path)
{
#ifdef _WIN32
	return GetFileAttributesW(Wide(path).c_str()) != INVALID_FILE_ATTRIBUTES;
#else
	struct stat st;
	return stat(path.c_str(), &st) == 0;
#endif
}

bool IsDirectory(const std::string& path)
{
#ifdef _WIN32
	const DWORD a = GetFileAttributesW(Wide(path).c_str());
	return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

bool MakeDirectories(const std::string& path)
{
	if (path.empty() || IsDirectory(path))
		return true;
	const size_t slash = path.find_last_of("/\\");
	if (slash != std::string::npos && slash > 0)
	{
		const std::string parent = path.substr(0, slash);
		// "C:" gibi sürücü kökünde dur
		if (!(parent.size() == 2 && parent[1] == ':') && !MakeDirectories(parent))
			return false;
	}
#ifdef _WIN32
	return CreateDirectoryW(Wide(path).c_str(), nullptr) != 0 || IsDirectory(path);
#else
	return mkdir(path.c_str(), 0755) == 0 || IsDirectory(path);
#endif
}

bool Rename(const std::string& from, const std::string& to)
{
#ifdef _WIN32
	return MoveFileExW(Wide(from).c_str(), Wide(to).c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
	return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}

bool CopyFileContents(const std::string& from, const std::string& to)
{
	auto data = ReadFile(from, 512u * 1024u * 1024u);
	if (!data)
		return false;
	return WriteFileAtomic(to, *data);
}

bool RemoveFile(const std::string& path)
{
#ifdef _WIN32
	return DeleteFileW(Wide(path).c_str()) != 0;
#else
	return unlink(path.c_str()) == 0;
#endif
}

std::vector<std::string> ListFiles(const std::string& dir)
{
	std::vector<std::string> out;
#ifdef _WIN32
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(Wide(Join(dir, "*")).c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
		return out;
	do
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			out.push_back(Narrow(fd.cFileName));
	} while (FindNextFileW(h, &fd));
	FindClose(h);
#else
	DIR* d = opendir(dir.c_str());
	if (!d)
		return out;
	while (dirent* e = readdir(d))
	{
		const std::string name = e->d_name;
		if (name == "." || name == "..")
			continue;
		if (!IsDirectory(Join(dir, name)))
			out.push_back(name);
	}
	closedir(d);
#endif
	return out;
}

std::string Join(const std::string& a, const std::string& b)
{
	if (a.empty())
		return b;
	const char last = a.back();
	if (last == '/' || last == '\\')
		return a + b;
	return a + "/" + b;
}

std::string TimestampForFilename()
{
	const std::time_t t = std::time(nullptr);
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif
	char buf[32];
	std::strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &tm);
	return buf;
}
} // namespace lambda::fileio
