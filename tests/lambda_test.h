// SPDX-License-Identifier: MIT
// Bağımlılıksız, küçük test çatısı.
#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace lambda_test
{
struct Case
{
	const char* name;
	std::function<void()> fn;
};

inline std::vector<Case>& Registry()
{
	static std::vector<Case> cases;
	return cases;
}

inline int& Failures()
{
	static int f = 0;
	return f;
}

struct Registrar
{
	Registrar(const char* name, std::function<void()> fn) { Registry().push_back({name, std::move(fn)}); }
};

inline void Fail(const char* file, int line, const std::string& msg)
{
	++Failures();
	std::printf("    FAILED %s:%d: %s\n", file, line, msg.c_str());
}
} // namespace lambda_test

#define LT_CAT2(a, b) a##b
#define LT_CAT(a, b) LT_CAT2(a, b)
#define TEST_CASE(name)                                                                   \
	static void LT_CAT(lt_fn_, __LINE__)();                                               \
	static ::lambda_test::Registrar LT_CAT(lt_reg_, __LINE__)(name, &LT_CAT(lt_fn_, __LINE__)); \
	static void LT_CAT(lt_fn_, __LINE__)()

#define CHECK(expr)                                                         \
	do                                                                      \
	{                                                                       \
		if (!(expr))                                                        \
			::lambda_test::Fail(__FILE__, __LINE__, "CHECK(" #expr ")"); \
	} while (0)

#define CHECK_NEAR(a, b, eps)                                                                              \
	do                                                                                                     \
	{                                                                                                      \
		const double lt_a = static_cast<double>(a), lt_b = static_cast<double>(b);                       \
		if (!(std::fabs(lt_a - lt_b) <= (eps)))                                                            \
			::lambda_test::Fail(__FILE__, __LINE__,                                                        \
				std::string("CHECK_NEAR(" #a ", " #b ") ") + std::to_string(lt_a) + " vs " + std::to_string(lt_b)); \
	} while (0)
