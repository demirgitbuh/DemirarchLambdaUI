// SPDX-License-Identifier: MIT
// lambda-setup: Demirarch Lambda UI kurulum yardımcısı.
//  status     Oyun yollarını ve mod dizinlerini doğrular (varsayılan)
//  install    Blue Shift haritalarının dönüştürülmüş kopyalarını lambda_bs/maps içine yazar
//  uninstall  Yalnızca modun kendi dizinlerini kaldırır (kayıtlar/profiller varsayılan olarak yedeklenir)
// Orijinal oyun dosyalarına hiçbir komut yazmaz.
#include "bsp_convert.h"

#include "lambda/core/fileio.h"
#include "lambda/version.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace lambda;

namespace
{
const char* const kModDirs[] = {"lambda_hl", "lambda_op4", "lambda_bs"};
constexpr const char* kToolsDir = "lambda_tools";
constexpr const char* kMarker = "lambda/lambda_mod.txt";

struct Options
{
	std::string command = "status";
	std::string halflife;
	bool yes = false;
	bool delete_user_data = false;
	bool force = false;
};

#ifdef _WIN32
std::wstring Wide(const std::string& s)
{
	const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring w(static_cast<size_t>(n > 0 ? n - 1 : 0), L'\0');
	if (n > 1)
		MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
	return w;
}

std::string Narrow(const wchar_t* w)
{
	const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
	std::string s(static_cast<size_t>(n > 0 ? n - 1 : 0), '\0');
	if (n > 1)
		WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
	return s;
}
#endif

std::string ExecutableDir()
{
#ifdef _WIN32
	wchar_t buf[4096];
	const DWORD n = GetModuleFileNameW(nullptr, buf, 4096);
	if (n == 0 || n >= 4096)
		return {};
	std::string p = Narrow(buf);
#else
	char buf[4096];
	const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (n <= 0)
		return {};
	buf[n] = '\0';
	std::string p = buf;
#endif
	const size_t slash = p.find_last_of("/\\");
	return slash == std::string::npos ? std::string{} : p.substr(0, slash);
}

std::string Parent(const std::string& p)
{
	const size_t slash = p.find_last_of("/\\");
	return slash == std::string::npos ? std::string{} : p.substr(0, slash);
}

bool LooksLikeHalfLife(const std::string& dir)
{
	if (dir.empty() || !fileio::Exists(fileio::Join(dir, "valve/liblist.gam")))
		return false;
	return fileio::Exists(fileio::Join(dir, "hl.exe")) || fileio::Exists(fileio::Join(dir, "hl_linux")) ||
		   fileio::Exists(fileio::Join(dir, "hl.sh"));
}

struct DirEntry
{
	std::string name;
	bool directory;
};

std::vector<DirEntry> ListAll(const std::string& dir)
{
	std::vector<DirEntry> out;
#ifdef _WIN32
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW(Wide(fileio::Join(dir, "*")).c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE)
		return out;
	do
	{
		const std::string name = Narrow(fd.cFileName);
		if (name == "." || name == "..")
			continue;
		// Bağlantı noktası/sembolik bağ izlenmez (silme işleminin dışarı taşmaması için).
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
			out.push_back({name, false});
		else
			out.push_back({name, (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0});
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
		struct stat st;
		const bool is_dir = lstat(fileio::Join(dir, name).c_str(), &st) == 0 && S_ISDIR(st.st_mode);
		out.push_back({name, is_dir});
	}
	closedir(d);
#endif
	return out;
}

bool RemoveTree(const std::string& dir)
{
	bool ok = true;
	for (const DirEntry& e : ListAll(dir))
	{
		const std::string p = fileio::Join(dir, e.name);
		if (e.directory)
			ok = RemoveTree(p) && ok;
		else
			ok = fileio::RemoveFile(p) && ok;
	}
#ifdef _WIN32
	ok = (RemoveDirectoryW(Wide(dir).c_str()) != 0) && ok;
#else
	ok = (rmdir(dir.c_str()) == 0) && ok;
#endif
	return ok;
}

void PrintHelp()
{
	std::printf(
		"%s setup tool %s\n\n"
		"Usage: lambda-setup [command] [options]\n\n"
		"Commands:\n"
		"  status      Check the Half-Life installation and the mod folders (default)\n"
		"  install     Create converted copies of your Blue Shift maps in lambda_bs/maps\n"
		"  uninstall   Remove only the Lambda UI folders (lambda_hl, lambda_op4, lambda_bs, lambda_tools)\n\n"
		"Options:\n"
		"  --halflife <folder>   Path to the Steam Half-Life folder (detected automatically if omitted)\n"
		"  --force               install: convert maps again even if copies already exist\n"
		"  --yes                 uninstall: confirm removal\n"
		"  --delete-user-data    uninstall: also delete Lambda UI saves and profiles (default: keep a backup)\n\n"
		"This tool never changes the original game folders.\n",
		LAMBDA_PRODUCT_NAME, LAMBDA_VERSION_STRING);
}

std::string FindHalfLife(const Options& opt)
{
	if (!opt.halflife.empty())
		return LooksLikeHalfLife(opt.halflife) ? opt.halflife : std::string{};
	const std::string exe = ExecutableDir();
	for (const std::string& candidate : {Parent(exe), exe})
		if (LooksLikeHalfLife(candidate))
			return candidate;
	return {};
}

int Status(const std::string& hl)
{
	std::printf("Half-Life folder: %s\n", hl.c_str());
	struct Game
	{
		const char* name;
		const char* dir;
		const char* mod;
	} games[] = {{"Half-Life", "valve", "lambda_hl"}, {"Opposing Force", "gearbox", "lambda_op4"}, {"Blue Shift", "bshift", "lambda_bs"}};
	int problems = 0;
	for (const Game& g : games)
	{
		const bool owned = fileio::Exists(fileio::Join(hl, std::string(g.dir) + "/liblist.gam"));
		const bool mod = fileio::Exists(fileio::Join(hl, std::string(g.mod) + "/liblist.gam"));
		std::printf("  %-15s game: %-13s Lambda UI folder: %s\n", g.name, owned ? "installed" : "not installed", mod ? "present" : "missing");
		if (mod && !owned)
		{
			std::printf("    The original game is required. Install %s from Steam and run it once.\n", g.name);
			++problems;
		}
	}
	const std::string bs_maps = fileio::Join(hl, "lambda_bs/maps");
	if (fileio::Exists(fileio::Join(hl, "lambda_bs/liblist.gam")) && fileio::Exists(fileio::Join(hl, "bshift/liblist.gam")))
	{
		const bool converted = fileio::Exists(fileio::Join(bs_maps, "ba_tram1.bsp"));
		std::printf("  Blue Shift maps: %s\n", converted ? "converted copies present" : "not converted yet - run: lambda-setup install");
		if (!converted)
			++problems;
	}
	return problems == 0 ? 0 : 2;
}

int Install(const std::string& hl, const Options& opt)
{
	const std::string src = fileio::Join(hl, "bshift/maps");
	const std::string dst = fileio::Join(hl, "lambda_bs/maps");
	if (!fileio::Exists(fileio::Join(hl, "lambda_bs/liblist.gam")))
	{
		std::printf("lambda_bs is not installed; nothing to do.\n");
		return 0;
	}
	if (!fileio::IsDirectory(src))
	{
		std::printf("Blue Shift is not installed (missing %s). Install it from Steam and run it once.\n", src.c_str());
		return 2;
	}
	if (!fileio::MakeDirectories(dst))
	{
		std::printf("Could not create %s.\n", dst.c_str());
		return 1;
	}
	int converted = 0, skipped = 0, failed = 0;
	for (const std::string& name : fileio::ListFiles(src))
	{
		if (name.size() < 5 || name.compare(name.size() - 4, 4, ".bsp") != 0)
			continue;
		const std::string out = fileio::Join(dst, name);
		if (!opt.force && fileio::Exists(out))
		{
			++skipped;
			continue;
		}
		auto data = fileio::ReadFile(fileio::Join(src, name), 256u * 1024u * 1024u);
		std::string error;
		if (!data || !setup::ConvertBlueShiftBsp(*data, &error))
		{
			std::printf("  %s: %s\n", name.c_str(), data ? error.c_str() : "could not be read");
			++failed;
			continue;
		}
		if (!fileio::WriteFileAtomic(out, *data))
		{
			std::printf("  %s: could not be written\n", name.c_str());
			++failed;
			continue;
		}
		++converted;
	}
	std::printf("Blue Shift maps: %d converted, %d already present, %d failed.\n", converted, skipped, failed);
	std::printf("Your original files in bshift/maps were not changed.\n");
	return failed == 0 ? 0 : 1;
}

int Uninstall(const std::string& hl, const Options& opt)
{
	std::vector<std::string> targets;
	for (const char* mod : kModDirs)
	{
		const std::string dir = fileio::Join(hl, mod);
		if (!fileio::IsDirectory(dir))
			continue;
		if (!fileio::Exists(fileio::Join(dir, kMarker)))
		{
			std::printf("Skipping %s: it does not contain the Lambda UI marker file (%s).\n", dir.c_str(), kMarker);
			continue;
		}
		targets.push_back(mod);
	}
	const bool tools = fileio::Exists(fileio::Join(hl, std::string(kToolsDir) + "/lambda_tools.txt"));
	if (targets.empty() && !tools)
	{
		std::printf("No Lambda UI folders were found in %s.\n", hl.c_str());
		return 0;
	}
	std::printf("The following folders will be removed from %s:\n", hl.c_str());
	for (const auto& t : targets)
		std::printf("  %s\n", t.c_str());
	if (tools)
		std::printf("  %s\n", kToolsDir);
	std::printf(opt.delete_user_data ? "Lambda UI saves and profiles will be DELETED.\n"
									 : "Lambda UI saves and profiles will be moved to a backup folder first.\n");
	if (!opt.yes)
	{
		std::printf("\nNothing was removed. Run again with --yes to confirm.\n");
		return 0;
	}

	if (!opt.delete_user_data)
	{
		const std::string backup = fileio::Join(hl, "lambda_userdata_backup-" + fileio::TimestampForFilename());
		for (const auto& t : targets)
		{
			for (const char* sub : {"SAVE", "lambda/profiles", "lambda/logs"})
			{
				const std::string from = fileio::Join(fileio::Join(hl, t), sub);
				if (!fileio::IsDirectory(from))
					continue;
				const std::string to = fileio::Join(fileio::Join(backup, t), sub);
				fileio::MakeDirectories(Parent(to));
				if (!fileio::Rename(from, to))
				{
					std::printf("Could not back up %s. Nothing else was removed.\n", from.c_str());
					return 1;
				}
			}
		}
		std::printf("User data backup: %s\n", backup.c_str());
	}

	bool ok = true;
	for (const auto& t : targets)
		ok = RemoveTree(fileio::Join(hl, t)) && ok;
	if (tools)
		std::printf("Remove the %s folder manually after this tool exits.\n", kToolsDir);
	std::printf(ok ? "Lambda UI was removed. Original game folders were not changed.\n"
				   : "Some files could not be removed. Close the game and try again.\n");
	return ok ? 0 : 1;
}
} // namespace

int main(int argc, char** argv)
{
	Options opt;
	for (int i = 1; i < argc; ++i)
	{
		const std::string a = argv[i];
		if (a == "--help" || a == "-h" || a == "help")
		{
			PrintHelp();
			return 0;
		}
		if (a == "--halflife" && i + 1 < argc)
			opt.halflife = argv[++i];
		else if (a == "--yes")
			opt.yes = true;
		else if (a == "--force")
			opt.force = true;
		else if (a == "--delete-user-data")
			opt.delete_user_data = true;
		else if (a == "status" || a == "install" || a == "uninstall")
			opt.command = a;
		else
		{
			std::printf("Unknown argument: %s\n\n", a.c_str());
			PrintHelp();
			return 64;
		}
	}

	const std::string hl = FindHalfLife(opt);
	if (hl.empty())
	{
		std::printf("The Steam Half-Life folder was not found.\n"
					"Extract the package into your Half-Life folder, or pass --halflife <folder>.\n");
		return 2;
	}
	if (opt.command == "install")
		return Install(hl, opt);
	if (opt.command == "uninstall")
		return Uninstall(hl, opt);
	return Status(hl);
}
