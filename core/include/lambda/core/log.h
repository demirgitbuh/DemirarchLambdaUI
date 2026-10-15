// SPDX-License-Identifier: MIT
// Yerel tanılama günlüğü. Hiçbir veri ağ üzerinden gönderilmez (otomatik telemetri yok).
//  - <mod>/lambda/logs/lambda.log, açılışta lambda.1.log'a döndürülür (2 dosya tutulur)
//  - Kişisel yollar yazılmadan önce arındırılır (kullanıcı ev dizini -> "~")
#pragma once

#include <string>

namespace lambda::log
{
enum class Level
{
	Debug,
	Info,
	Warning,
	Error,
};

void Open(const std::string& directory);
void Close();
bool IsOpen();
void SetMinimumLevel(Level level);
void Write(Level level, const char* format, ...)
#if defined(__GNUC__)
	__attribute__((format(printf, 2, 3)))
#endif
	;

// Ev dizini, kullanıcı adı gibi kişisel bilgileri metinden çıkarır.
std::string Sanitize(const std::string& text);
const std::string& Path();
} // namespace lambda::log

#define LAMBDA_LOG_INFO(...) ::lambda::log::Write(::lambda::log::Level::Info, __VA_ARGS__)
#define LAMBDA_LOG_WARN(...) ::lambda::log::Write(::lambda::log::Level::Warning, __VA_ARGS__)
#define LAMBDA_LOG_ERROR(...) ::lambda::log::Write(::lambda::log::Level::Error, __VA_ARGS__)
#define LAMBDA_LOG_DEBUG(...) ::lambda::log::Write(::lambda::log::Level::Debug, __VA_ARGS__)
