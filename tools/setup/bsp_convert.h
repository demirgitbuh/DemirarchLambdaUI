// SPDX-License-Identifier: MIT
// Blue Shift BSP başlık dönüştürmesi.
// Blue Shift haritalarında başlıktaki ilk iki lump (entities ve planes) yer değiştirmiştir;
// motor bu düzeni yalnızca oyun dizini "bshift" iken tanır. Ayrı mod dizininden yüklenebilmesi
// için kullanıcının kendi haritasının bir KOPYASINDA bu iki giriş takas edilir. Orijinal dosyaya
// dokunulmaz.
#pragma once

#include <cstdint>
#include <string>

namespace lambda::setup
{
enum class BspLayout
{
	Invalid,
	Standard,  // lump0 = entities (metin)
	BlueShift, // lump0 = planes, lump1 = entities
};

struct BspInspect
{
	BspLayout layout = BspLayout::Invalid;
	std::string error;
};

BspInspect InspectBsp(const std::string& data);
// data'yı standart düzene çevirir. Zaten standartsa değiştirmeden true döner.
bool ConvertBlueShiftBsp(std::string& data, std::string* error);
} // namespace lambda::setup
