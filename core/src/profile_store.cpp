// SPDX-License-Identifier: MIT
#include "lambda/core/profile_store.h"

#include "lambda/core/fileio.h"

#include <algorithm>

namespace lambda
{
namespace
{
constexpr const char* kExt = ".cfg";

std::string BackupPath(const std::string& primary, int index)
{
	return primary + ".bak" + std::to_string(index);
}

std::optional<SettingsDocument> TryRead(const std::string& path, std::vector<ParseIssue>* issues)
{
	auto text = fileio::ReadFile(path);
	if (!text)
		return std::nullopt;
	auto doc = SettingsDocument::Parse(*text, issues);
	if (!doc)
		return std::nullopt;
	std::string err;
	if (!CheckDocument(*doc, "profile", &err))
		return std::nullopt;
	return doc;
}
} // namespace

void StampDocument(SettingsDocument& doc, const std::string& kind)
{
	doc.SetInt("meta", "format", ProfileStore::kFormatVersion);
	doc.Set("meta", "kind", kind);
}

bool CheckDocument(const SettingsDocument& doc, const std::string& kind, std::string* error)
{
	const int format = doc.GetInt("meta", "format", -1);
	if (format < 1)
	{
		if (error)
			*error = "The file is not a Demirarch Lambda UI profile.";
		return false;
	}
	if (format > ProfileStore::kFormatVersion)
	{
		if (error)
			*error = "The file was created by a newer version of Demirarch Lambda UI.";
		return false;
	}
	if (doc.GetString("meta", "kind", "") != kind)
	{
		if (error)
			*error = "The file does not contain a controller profile.";
		return false;
	}
	return true;
}

ProfileStore::ProfileStore(std::string directory) : directory_(std::move(directory)) {}

bool ProfileStore::IsValidProfileName(const std::string& name)
{
	if (name.empty() || name.size() > 64)
		return false;
	for (char c : name)
	{
		const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
		if (!ok)
			return false;
	}
	return true;
}

std::string ProfileStore::PathFor(const std::string& name) const { return fileio::Join(directory_, name + kExt); }

bool ProfileStore::Exists(const std::string& name) const { return IsValidProfileName(name) && fileio::Exists(PathFor(name)); }

ProfileLoadResult ProfileStore::Load(const std::string& name) const
{
	ProfileLoadResult r;
	if (!IsValidProfileName(name))
	{
		r.source = LoadSource::Failed;
		r.message = "Invalid profile name.";
		return r;
	}
	const std::string primary = PathFor(name);
	const bool primary_exists = fileio::Exists(primary);

	if (primary_exists)
	{
		if (auto doc = TryRead(primary, &r.issues))
		{
			r.source = LoadSource::Primary;
			r.document = std::move(*doc);
			return r;
		}
		// Bozuk: silme, karantinaya al.
		const std::string quarantine = fileio::Join(directory_, name + ".corrupt-" + fileio::TimestampForFilename() + kExt);
		if (fileio::Rename(primary, quarantine))
			r.quarantined_path = quarantine;
	}

	for (int i = 1; i <= kBackupCount; ++i)
	{
		std::vector<ParseIssue> issues;
		if (auto doc = TryRead(BackupPath(primary, i), &issues))
		{
			r.source = LoadSource::Backup;
			r.backup_index = i;
			r.document = std::move(*doc);
			r.issues = std::move(issues);
			r.message = primary_exists ? "The profile \"" + name + "\" was damaged. A backup was restored; the damaged file was kept as " +
											 (r.quarantined_path.empty() ? std::string("is") : r.quarantined_path) + "."
									   : "The profile \"" + name + "\" was missing. A backup was restored.";
			return r;
		}
	}

	if (primary_exists)
	{
		r.source = LoadSource::Failed;
		r.message = "The profile \"" + name + "\" could not be read and no usable backup was found. Default settings are in use; the damaged file was kept.";
	}
	else
	{
		r.source = LoadSource::NotFound;
	}
	return r;
}

bool ProfileStore::Save(const std::string& name, const SettingsDocument& source) const
{
	if (!IsValidProfileName(name) || !fileio::MakeDirectories(directory_))
		return false;
	SettingsDocument doc = source;
	StampDocument(doc, "profile");
	const std::string primary = PathFor(name);

	// Yedekleri döndür: bak2 -> bak3, bak1 -> bak2, ana -> bak1 (kopya; ana dosya yerinde kalır).
	if (fileio::Exists(primary))
	{
		for (int i = kBackupCount - 1; i >= 1; --i)
		{
			const std::string from = BackupPath(primary, i);
			if (fileio::Exists(from))
				fileio::Rename(from, BackupPath(primary, i + 1));
		}
		// Yalnızca geçerli bir ana dosya yedeğe alınır; bozuk dosya yedekleri ezmesin.
		if (TryRead(primary, nullptr))
			fileio::CopyFileContents(primary, BackupPath(primary, 1));
	}
	return fileio::WriteFileAtomic(primary, "# Demirarch Lambda UI controller profile\n" + doc.Serialize());
}

std::vector<std::string> ProfileStore::List() const
{
	std::vector<std::string> names;
	for (const std::string& file : fileio::ListFiles(directory_))
	{
		if (file.size() <= 4 || file.compare(file.size() - 4, 4, kExt) != 0)
			continue;
		const std::string name = file.substr(0, file.size() - 4);
		if (IsValidProfileName(name))
			names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	return names;
}

bool ProfileStore::Export(const std::string& name, const std::string& destination_path) const
{
	auto r = Load(name);
	if (r.source != LoadSource::Primary && r.source != LoadSource::Backup)
		return false;
	return fileio::WriteFileAtomic(destination_path, "# Demirarch Lambda UI controller profile (exported)\n" + r.document.Serialize());
}

std::optional<SettingsDocument> ProfileStore::ReadImport(const std::string& source_path, std::string* error)
{
	auto text = fileio::ReadFile(source_path);
	if (!text)
	{
		if (error)
			*error = "The file could not be opened.";
		return std::nullopt;
	}
	auto doc = SettingsDocument::Parse(*text);
	if (!doc)
	{
		if (error)
			*error = "The file is damaged or is not a profile.";
		return std::nullopt;
	}
	if (!CheckDocument(*doc, "profile", error))
		return std::nullopt;
	return doc;
}
} // namespace lambda
