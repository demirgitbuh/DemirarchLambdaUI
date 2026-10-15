// SPDX-License-Identifier: MIT
// Profil dosyalarının güvenli saklanması:
//  - Atomik yazma (tmp + yeniden adlandırma)
//  - Dönen yedekler: <ad>.bak1 (en yeni) .. <ad>.bak3
//  - Bozuk dosya asla silinmez: <ad>.corrupt-<zaman>.cfg olarak karantinaya alınır,
//    en yeni geçerli yedek yüklenir ve kullanıcıya bildirilir.
//  - İçe/dışa aktarma
#pragma once

#include "lambda/core/document.h"

#include <optional>
#include <string>
#include <vector>

namespace lambda
{
enum class LoadSource
{
	Primary,
	Backup,   // ana dosya bozuktu/eksikti, yedekten yüklendi
	NotFound, // dosya yok (ilk çalıştırma)
	Failed,   // ana dosya ve tüm yedekler okunamadı
};

struct ProfileLoadResult
{
	LoadSource source = LoadSource::NotFound;
	SettingsDocument document;
	int backup_index = 0;               // Backup ise kullanılan yedek
	std::string quarantined_path;       // bozuk ana dosyanın yeni adı
	std::vector<ParseIssue> issues;     // tolere edilen satır hataları
	std::string message;                // kullanıcıya gösterilecek İngilizce açıklama
};

class ProfileStore
{
public:
	static constexpr int kBackupCount = 3;
	static constexpr int kFormatVersion = 1;

	explicit ProfileStore(std::string directory);

	const std::string& Directory() const { return directory_; }
	std::string PathFor(const std::string& name) const;
	static bool IsValidProfileName(const std::string& name);

	ProfileLoadResult Load(const std::string& name) const;
	bool Save(const std::string& name, const SettingsDocument& doc) const;
	bool Exists(const std::string& name) const;
	std::vector<std::string> List() const;

	bool Export(const std::string& name, const std::string& destination_path) const;
	// Dışarıdan gelen dosyayı doğrular; geçerliyse belgeyi döndürür (kaydetmez).
	static std::optional<SettingsDocument> ReadImport(const std::string& source_path, std::string* error);

private:
	std::string directory_;
};

// Belge biçim sürümünü ve türünü yazar/denetler.
void StampDocument(SettingsDocument& doc, const std::string& kind);
bool CheckDocument(const SettingsDocument& doc, const std::string& kind, std::string* error);
} // namespace lambda
