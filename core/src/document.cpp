// SPDX-License-Identifier: MIT
#include "lambda/core/document.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace lambda
{
std::string_view Trim(std::string_view s)
{
	while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
		s.remove_prefix(1);
	while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
		s.remove_suffix(1);
	return s;
}

// Yerel ayardan (ondalık virgül vb.) bağımsız ayrıştırma; motor ya da sistem setlocale
// çağırsa bile profil dosyaları aynı okunur.
std::optional<float> ParseFloatInvariant(std::string_view s)
{
	s = Trim(s);
	if (s.empty() || s.size() > 48)
		return std::nullopt;
	size_t i = 0;
	bool neg = false;
	if (s[i] == '+' || s[i] == '-')
		neg = s[i++] == '-';
	double value = 0.0;
	int digits = 0;
	while (i < s.size() && s[i] >= '0' && s[i] <= '9')
	{
		value = value * 10.0 + (s[i++] - '0');
		++digits;
	}
	if (i < s.size() && s[i] == '.')
	{
		++i;
		double scale = 0.1;
		while (i < s.size() && s[i] >= '0' && s[i] <= '9')
		{
			value += (s[i++] - '0') * scale;
			scale *= 0.1;
			++digits;
		}
	}
	if (digits == 0)
		return std::nullopt;
	if (i < s.size() && (s[i] == 'e' || s[i] == 'E'))
	{
		++i;
		bool eneg = false;
		if (i < s.size() && (s[i] == '+' || s[i] == '-'))
			eneg = s[i++] == '-';
		int exp = 0;
		int edigits = 0;
		while (i < s.size() && s[i] >= '0' && s[i] <= '9' && exp < 400)
		{
			exp = exp * 10 + (s[i++] - '0');
			++edigits;
		}
		if (edigits == 0)
			return std::nullopt;
		value *= std::pow(10.0, eneg ? -exp : exp);
	}
	if (i != s.size() || !std::isfinite(value) || value > 3.0e38)
		return std::nullopt;
	return static_cast<float>(neg ? -value : value);
}

std::string FormatFloatInvariant(float value)
{
	if (!std::isfinite(value))
		value = 0.0f;
	char buf[64];
	std::snprintf(buf, sizeof(buf), "%.6g", static_cast<double>(value));
	std::string out(buf);
	for (char& c : out)
		if (c == ',')
			c = '.';
	return out;
}

namespace
{
bool ValidName(std::string_view s)
{
	if (s.empty() || s.size() > 128)
		return false;
	for (char c : s)
	{
		const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '.' ||
						c == '-' || c == '+';
		if (!ok)
			return false;
	}
	return true;
}
} // namespace

std::optional<SettingsDocument> SettingsDocument::Parse(std::string_view text, std::vector<ParseIssue>* issues)
{
	SettingsDocument doc;
	// UTF-8 BOM
	if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF && static_cast<unsigned char>(text[1]) == 0xBB &&
		static_cast<unsigned char>(text[2]) == 0xBF)
		text.remove_prefix(3);

	// İkili/bozuk veri: NUL baytı içeren dosya geçersiz sayılır.
	if (text.find('\0') != std::string_view::npos)
	{
		if (issues)
			issues->push_back({0, "File contains binary data."});
		return std::nullopt;
	}

	std::string current = "";
	int line_no = 0;
	size_t pos = 0;
	int bad_lines = 0;
	int good_lines = 0;
	while (pos <= text.size())
	{
		size_t end = text.find('\n', pos);
		if (end == std::string_view::npos)
			end = text.size();
		std::string_view line = Trim(text.substr(pos, end - pos));
		++line_no;
		pos = end + 1;

		if (line.empty() || line.front() == '#' || line.front() == ';')
		{
			if (end == text.size())
				break;
			continue;
		}
		if (line.front() == '[')
		{
			if (line.back() != ']' || !ValidName(Trim(line.substr(1, line.size() - 2))))
			{
				++bad_lines;
				if (issues)
					issues->push_back({line_no, "Invalid section header."});
			}
			else
			{
				current = std::string(Trim(line.substr(1, line.size() - 2)));
				doc.EnsureSection(current);
				++good_lines;
			}
		}
		else
		{
			const size_t eq = line.find('=');
			const std::string_view key = eq == std::string_view::npos ? std::string_view{} : Trim(line.substr(0, eq));
			if (eq == std::string_view::npos || !ValidName(key) || current.empty())
			{
				++bad_lines;
				if (issues)
					issues->push_back({line_no, "Invalid entry."});
			}
			else
			{
				doc.Set(current, key, Trim(line.substr(eq + 1)));
				++good_lines;
			}
		}
		if (end == text.size())
			break;
	}

	// Çoğunluğu bozuk satırlardan oluşan dosya güvenilmezdir; çağıran yedeğe döner.
	if (bad_lines > 0 && bad_lines >= good_lines)
		return std::nullopt;
	return doc;
}

std::string SettingsDocument::Serialize() const
{
	std::string out;
	for (const Section& s : sections_)
	{
		if (!out.empty())
			out += '\n';
		out += '[';
		out += s.name;
		out += "]\n";
		for (const auto& [k, v] : s.entries)
		{
			out += k;
			out += " = ";
			out += v;
			out += '\n';
		}
	}
	return out;
}

bool SettingsDocument::HasSection(std::string_view section) const { return GetSection(section) != nullptr; }

const SettingsDocument::Section* SettingsDocument::GetSection(std::string_view section) const
{
	for (const Section& s : sections_)
		if (s.name == section)
			return &s;
	return nullptr;
}

void SettingsDocument::RemoveSection(std::string_view section)
{
	for (auto it = sections_.begin(); it != sections_.end(); ++it)
	{
		if (it->name == section)
		{
			sections_.erase(it);
			return;
		}
	}
}

SettingsDocument::Section& SettingsDocument::EnsureSection(std::string_view section)
{
	for (Section& s : sections_)
		if (s.name == section)
			return s;
	sections_.push_back(Section{std::string(section), {}});
	return sections_.back();
}

std::optional<std::string> SettingsDocument::Get(std::string_view section, std::string_view key) const
{
	if (const Section* s = GetSection(section))
		for (const auto& [k, v] : s->entries)
			if (k == key)
				return v;
	return std::nullopt;
}

std::string SettingsDocument::GetString(std::string_view section, std::string_view key, std::string_view fallback) const
{
	auto v = Get(section, key);
	return v ? *v : std::string(fallback);
}

float SettingsDocument::GetFloat(std::string_view section, std::string_view key, float fallback) const
{
	auto v = Get(section, key);
	if (!v || v->empty())
		return fallback;
	const auto parsed = ParseFloatInvariant(*v);
	return parsed ? *parsed : fallback;
}

int SettingsDocument::GetInt(std::string_view section, std::string_view key, int fallback) const
{
	auto v = Get(section, key);
	if (!v || v->empty())
		return fallback;
	char* end = nullptr;
	errno = 0;
	const long l = std::strtol(v->c_str(), &end, 10);
	if (errno != 0 || end == v->c_str() || *end != '\0' || l < -2147483647L || l > 2147483647L)
		return fallback;
	return static_cast<int>(l);
}

bool SettingsDocument::GetBool(std::string_view section, std::string_view key, bool fallback) const
{
	auto v = Get(section, key);
	if (!v)
		return fallback;
	if (*v == "1" || *v == "true" || *v == "yes" || *v == "on")
		return true;
	if (*v == "0" || *v == "false" || *v == "no" || *v == "off")
		return false;
	return fallback;
}

void SettingsDocument::Set(std::string_view section, std::string_view key, std::string_view value)
{
	Section& s = EnsureSection(section);
	// Değerlerde satır sonu olamaz; tek satıra indirilir.
	std::string clean(value);
	for (char& c : clean)
		if (c == '\n' || c == '\r')
			c = ' ';
	for (auto& [k, v] : s.entries)
	{
		if (k == key)
		{
			v = std::move(clean);
			return;
		}
	}
	s.entries.emplace_back(std::string(key), std::move(clean));
}

void SettingsDocument::SetFloat(std::string_view section, std::string_view key, float value)
{
	Set(section, key, FormatFloatInvariant(value));
}

void SettingsDocument::SetInt(std::string_view section, std::string_view key, int value) { Set(section, key, std::to_string(value)); }

void SettingsDocument::SetBool(std::string_view section, std::string_view key, bool value) { Set(section, key, value ? "1" : "0"); }

void SettingsDocument::Overlay(const SettingsDocument& other)
{
	for (const Section& s : other.sections_)
		for (const auto& [k, v] : s.entries)
			Set(s.name, k, v);
}
} // namespace lambda
