// SPDX-License-Identifier: MIT
#include "lambda_test.h"

#include <cstring>

int main(int argc, char** argv)
{
	const char* filter = argc > 1 ? argv[1] : nullptr;
	int run = 0;
	for (const auto& c : lambda_test::Registry())
	{
		if (filter && !std::strstr(c.name, filter))
			continue;
		const int before = lambda_test::Failures();
		c.fn();
		++run;
		std::printf("%s %s\n", lambda_test::Failures() == before ? "[ OK ]" : "[FAIL]", c.name);
	}
	std::printf("\n%d test case(s), %d failed check(s)\n", run, lambda_test::Failures());
	return lambda_test::Failures() == 0 ? 0 : 1;
}
