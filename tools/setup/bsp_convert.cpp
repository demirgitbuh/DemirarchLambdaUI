// SPDX-License-Identifier: MIT
#include "bsp_convert.h"

#include <cstring>

namespace lambda::setup
{
namespace
{
constexpr int kBspVersion = 30;
constexpr int kLumpCount = 15;
constexpr size_t kHeaderSize = 4 + kLumpCount * 8;
constexpr int kPlaneSize = 20; // dplane_t: normal[3] float + dist float + type int

std::int32_t ReadI32(const std::string& d, size_t off)
{
	std::int32_t v;
	std::memcpy(&v, d.data() + off, 4); // BSP little-endian; hedef platformlar da little-endian
	return v;
}

void WriteI32(std::string& d, size_t off, std::int32_t v) { std::memcpy(&d[off], &v, 4); }

bool LumpInRange(const std::string& d, std::int32_t ofs, std::int32_t len)
{
	return ofs >= 0 && len >= 0 && static_cast<std::uint64_t>(ofs) + static_cast<std::uint64_t>(len) <= d.size();
}

// Entity lump'ı "{" ile başlayan metindir (öncesinde boşluk olabilir).
bool LooksLikeEntities(const std::string& d, std::int32_t ofs, std::int32_t len)
{
	if (len <= 0 || !LumpInRange(d, ofs, len))
		return false;
	for (std::int32_t i = 0; i < len && i < 64; ++i)
	{
		const char c = d[static_cast<size_t>(ofs + i)];
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
			continue;
		return c == '{';
	}
	return false;
}
} // namespace

BspInspect InspectBsp(const std::string& d)
{
	BspInspect r;
	if (d.size() < kHeaderSize)
	{
		r.error = "File is too small to be a BSP map.";
		return r;
	}
	if (ReadI32(d, 0) != kBspVersion)
	{
		r.error = "Unsupported BSP version.";
		return r;
	}
	const std::int32_t ofs0 = ReadI32(d, 4), len0 = ReadI32(d, 8);
	const std::int32_t ofs1 = ReadI32(d, 12), len1 = ReadI32(d, 16);
	if (!LumpInRange(d, ofs0, len0) || !LumpInRange(d, ofs1, len1))
	{
		r.error = "BSP header points outside the file.";
		return r;
	}
	if (LooksLikeEntities(d, ofs0, len0))
		r.layout = BspLayout::Standard;
	else if (LooksLikeEntities(d, ofs1, len1) && len0 % kPlaneSize == 0)
		r.layout = BspLayout::BlueShift;
	else
		r.error = "Could not identify the BSP layout.";
	return r;
}

bool ConvertBlueShiftBsp(std::string& d, std::string* error)
{
	const BspInspect info = InspectBsp(d);
	if (info.layout == BspLayout::Invalid)
	{
		if (error)
			*error = info.error;
		return false;
	}
	if (info.layout == BspLayout::Standard)
		return true;
	const std::int32_t ofs0 = ReadI32(d, 4), len0 = ReadI32(d, 8);
	const std::int32_t ofs1 = ReadI32(d, 12), len1 = ReadI32(d, 16);
	WriteI32(d, 4, ofs1);
	WriteI32(d, 8, len1);
	WriteI32(d, 12, ofs0);
	WriteI32(d, 16, len0);
	return InspectBsp(d).layout == BspLayout::Standard;
}
} // namespace lambda::setup
