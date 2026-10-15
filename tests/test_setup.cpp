// SPDX-License-Identifier: MIT
#include "lambda_test.h"

#include "bsp_convert.h"

#include <cstring>

using namespace lambda::setup;

namespace
{
// En küçük geçerli BSP: başlık + iki lump.
std::string MakeBsp(bool blue_shift)
{
	const std::string entities = "{\n\"classname\" \"worldspawn\"\n}\n";
	const std::string planes(40, '\x01'); // 2 düzlem x 20 bayt
	const int header = 4 + 15 * 8;
	std::string d(static_cast<size_t>(header), '\0');
	int version = 30;
	std::memcpy(&d[0], &version, 4);
	auto put = [&](int lump, int ofs, int len) {
		std::memcpy(&d[4 + lump * 8], &ofs, 4);
		std::memcpy(&d[8 + lump * 8], &len, 4);
	};
	const int planes_ofs = header;
	const int ent_ofs = header + static_cast<int>(planes.size());
	d += planes;
	d += entities;
	if (blue_shift)
	{
		put(0, planes_ofs, static_cast<int>(planes.size()));
		put(1, ent_ofs, static_cast<int>(entities.size()));
	}
	else
	{
		put(0, ent_ofs, static_cast<int>(entities.size()));
		put(1, planes_ofs, static_cast<int>(planes.size()));
	}
	return d;
}
} // namespace

TEST_CASE("setup: Blue Shift BSP header is converted to the standard layout")
{
	std::string bs = MakeBsp(true);
	CHECK(InspectBsp(bs).layout == BspLayout::BlueShift);
	std::string err;
	CHECK(ConvertBlueShiftBsp(bs, &err));
	CHECK(InspectBsp(bs).layout == BspLayout::Standard);
	CHECK(bs == MakeBsp(false));
}

TEST_CASE("setup: standard maps are left untouched and invalid files are rejected")
{
	std::string hl = MakeBsp(false);
	const std::string copy = hl;
	std::string err;
	CHECK(ConvertBlueShiftBsp(hl, &err));
	CHECK(hl == copy);

	std::string tiny = "abc";
	CHECK(!ConvertBlueShiftBsp(tiny, &err));
	std::string wrong = MakeBsp(true);
	int v = 29;
	std::memcpy(&wrong[0], &v, 4);
	CHECK(!ConvertBlueShiftBsp(wrong, &err));
	std::string out_of_range = MakeBsp(true);
	int huge = 1 << 30;
	std::memcpy(&out_of_range[4], &huge, 4);
	CHECK(InspectBsp(out_of_range).layout == BspLayout::Invalid);
}
