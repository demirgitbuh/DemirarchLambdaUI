// SPDX-License-Identifier: MIT
// Platformdan bağımsız küçük dosya işlemleri. Yollar UTF-8'dir (Windows'ta geniş API'ye çevrilir).
#pragma once

#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace lambda::fileio
{
// UTF-8 yolla dosya açar (Windows'ta geniş API). mode: "r", "w", "rb", "wb", "a"
std::FILE* OpenFile(const std::string& path, const char* mode);
std::optional<std::string> ReadFile(const std::string& path, size_t max_bytes = 4 * 1024 * 1024);
// Önce <path>.tmp yazılır, diske boşaltılır, sonra hedefin üzerine atomik olarak taşınır.
bool WriteFileAtomic(const std::string& path, const std::string& contents);
bool Exists(const std::string& path);
bool IsDirectory(const std::string& path);
bool MakeDirectories(const std::string& path);
// Hedef varsa üzerine yazar.
bool Rename(const std::string& from, const std::string& to);
bool CopyFileContents(const std::string& from, const std::string& to);
bool RemoveFile(const std::string& path);
// Dizindeki dosya adları (alt dizinler hariç).
std::vector<std::string> ListFiles(const std::string& dir);
std::string Join(const std::string& a, const std::string& b);
// Yerel saatle "YYYYMMDD-HHMMSS"
std::string TimestampForFilename();
} // namespace lambda::fileio
