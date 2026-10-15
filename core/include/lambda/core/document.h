// SPDX-License-Identifier: MIT
// Basit, insan tarafından okunabilir ayar belgesi:
//   # yorum
//   [bolum]
//   anahtar = değer
// Bilinmeyen bölüm/anahtarlar korunur (ileri uyumluluk); sıra korunur.
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lambda
{
struct ParseIssue
{
	int line = 0;
	std::string message;
};

class SettingsDocument
{
public:
	using Entry = std::pair<std::string, std::string>;
	struct Section
	{
		std::string name;
		std::vector<Entry> entries;
	};

	static std::optional<SettingsDocument> Parse(std::string_view text, std::vector<ParseIssue>* issues = nullptr);
	std::string Serialize() const;

	bool HasSection(std::string_view section) const;
	const Section* GetSection(std::string_view section) const;
	void RemoveSection(std::string_view section);
	const std::vector<Section>& Sections() const { return sections_; }

	std::optional<std::string> Get(std::string_view section, std::string_view key) const;
	std::string GetString(std::string_view section, std::string_view key, std::string_view fallback) const;
	float GetFloat(std::string_view section, std::string_view key, float fallback) const;
	int GetInt(std::string_view section, std::string_view key, int fallback) const;
	bool GetBool(std::string_view section, std::string_view key, bool fallback) const;

	void Set(std::string_view section, std::string_view key, std::string_view value);
	void SetFloat(std::string_view section, std::string_view key, float value);
	void SetInt(std::string_view section, std::string_view key, int value);
	void SetBool(std::string_view section, std::string_view key, bool value);

	// other'daki tüm değerleri bunun üzerine yazar (profil türetme / katmanlama).
	void Overlay(const SettingsDocument& other);

private:
	Section& EnsureSection(std::string_view section);
	std::vector<Section> sections_;
};

std::string_view Trim(std::string_view s);
std::optional<float> ParseFloatInvariant(std::string_view s);
std::string FormatFloatInvariant(float value);
} // namespace lambda
