# Aşama 2: Entegrasyon Planı

Tarih: 2026-10-16. Kararlar için bkz. `01-feasibility.md` bölüm 9.

## 1. Ek bulgular (Aşama 1'den sonra doğrulandı)

- **Blue Shift BSP biçimi:** `bshift/maps/ba_tram1.bsp` başlığında ilk lump'ın ofseti 124. Bu, düzlemlerin (planes) önce, varlıkların (entities) sonra geldiğini gösterir. Standart HL BSP'de sıra tersidir. Motor bu düzeni yalnızca oyun dizini adı `bshift` iken tanıyor; `halflife-bs-updated` de bu yüzden haritaları kurulumda kopyalayıp başlıktaki ilk iki lump'ı takas ediyor (`bspconverter/.../Main.cpp`).
  - **Sonuç:** `lambda_bs` modu, kullanıcının kendi `bshift/maps/*.bsp` dosyalarının **dönüştürülmüş kopyalarına** ihtiyaç duyar. Dönüştürme kullanıcının makinesinde, kurulum aracıyla yapılır. Orijinal dosyalara dokunulmaz ve dönüştürülmüş haritalar pakete girmez.
- `valve/maps` 127, `gearbox/maps` 68, `bshift/maps` 37 gevşek `.bsp` dosyası içeriyor; `.pak` yok.
- Updated depolarından yalnızca `halflife-updated` CMake desteğine sahip; OF/BS yalnızca VS projeleri ve Makefile içeriyor. Üç oyun için tek tip CMake yazılacak.
- Upstream CI, Linux için `registry.gitlab.steamos.cloud/steamrt/scout/sdk/i386` + `g++-9` kullanıyor. Oyunla uyumu kanıtlanmış bu tabanı koruyacağız.

## 2. Depo düzeni

```
DemirarchLambdaUI/
  core/            MIT — motordan bağımsız C++17: girdi işleme, bağlamalar, profiller,
                   mod durumu, nişan yardımı matematiği, çark, UI odak modeli, metin girişi,
                   haptik/tetik/LED öncelik mantığı. Birim testleri: core/tests
  platform/        MIT — SDL3 arka ucu (cihaz, hot-plug, gyro, touchpad, LED, tetik, pil),
                   DualSense USB haptik (miniaudio), Steam flat API köprüsü
  bridge/          HL SDK Lisansı — GoldSrc istemci/sunucu köprüsü (üç oyunda ortak):
                   VGUI2 birlikte çalışabilirlik katmanı, OpenGL çizici, HUD, menüler,
                   konsol, kayıt/yükleme, silah çarkı, hlclassic/hlgamepad komutları
  games/hl         halflife-updated @ 1665e7c (HL SDK Lisansı) + Lambda kancaları
  games/opfor      halflife-op4-updated @ 5368be7 + Lambda kancaları
  games/bshift     halflife-bs-updated @ 3ec5a23 + Lambda kancaları
  tools/setup      MIT — lambda-setup: oyun yolu doğrulama, BS harita dönüştürme,
                   kaldırma, destek paketi
  third_party/     SDL3 (zlib), miniaudio (MIT-0) — sürümleri sabitlenmiş
  gamedir/         Paket içeriği: liblist.gam, resource, gfx (yalnızca özgün varlıklar)
  docs/            Kullanıcı belgeleri (İngilizce) + docs/dev (Türkçe geliştirme notları)
```

## 3. Mod dizinleri

| Mod dizini | Taban | liblist | Not |
|---|---|---|---|
| `lambda_hl` | `valve` (motorun her zaman eklediği taban) | `game "Half-Life: Lambda UI"` | Harita kopyası yok |
| `lambda_op4` | `fallback_dir "gearbox"` | `game "Opposing Force: Lambda UI"` | OF çok oyunculu bu moddan |
| `lambda_bs` | `fallback_dir "bshift"` | `game "Blue Shift: Lambda UI"`, `type "singleplayer_only"` | Dönüştürülmüş haritalar `lambda_bs/maps` içinde |

## 4. İstemci kancaları (her oyunun `cl_dll`'inde, asgari dokunuş)

| Kanca noktası | Amaç |
|---|---|
| `CreateInterface` dışa aktarımı | `VClientVGUI001` → kendi kök panelimiz (menüde de çizim) |
| `Initialize` / `HUD_Init` / `HUD_VidInit` / `HUD_Shutdown` | Çekirdeğin yaşam döngüsü, `hlclassic`/`hlgamepad` komutları, cvar'lar |
| `HUD_Frame` | SDL3 olay pompası, hot-plug, haptik/LED güncellemesi |
| `HUD_Redraw` | Yeni HUD (gamepad modu) / özgün HUD (klasik mod) |
| `CL_CreateMove` | Analog hareket, kamera, nişan yardımı, oynanış yardımcıları → `usercmd_t` |
| `IN_MouseMove` / `IN_Accumulate` | Fare kullanımının saptanması (yardımın devreden çıkması) |
| `HUD_Key_Event` | UI açıkken oynanış tuşlarının yutulması; kurtarma kombinasyonu |
| `IN_Commands` / `IN_StartupJoystick` | Klasik modda SDK'nın özgün SDL2 yolu; gamepad modunda devre dışı |
| Kullanıcı mesajları (`WeaponList`, `CurWeapon`, `AmmoX`, `Health`, `Damage`, …) | Çark, HUD, haptik olayları |

## 5. Sunucu kancaları

- Mermi yönü yardımı: `CBasePlayer::GetAutoaimVector` / `FireBulletsPlayer` çevresinde, **yalnızca** `!g_pGameRules->IsMultiplayer()` ve istemcinin bildirdiği ayar açıkken.
- Hedef doğrulama: `UTIL_TraceLine` (görüş hattı), `pev->takedamage`, `IsAlive()`, `pev->deadflag`, sınıf ilişkisi (`IRelationship`) → dost/düşman ayrımı.
- İstemci ayarları sunucuya `setinfo` anahtarları ile iletilir (`la_aa`, `la_bd`); sunucu çok oyunculuda bunları **yok sayar** (ikinci kat güvence).

## 6. Prototipte çalışma zamanında doğrulanacaklar (kullanıcı makinesinde)

1. IClientVGUI kök panelinin ana menüde çizilmesi ve GameUI üzerindeki z-sırası.
2. `IBaseUI` ile GameUI gizleme/gösterme; ESC davranışı.
3. `sys_timescale`'in tek oyunculuda cheat olmadan çalışıp çalışmadığı.
4. `-lambdasafe` gibi özel parametrelerin `CheckParm` ile görülmesi.
5. Konsol çıktısının yakalanma yolu.
6. Motorun ISteamInput kullanımı ve Steam Input açıkken SDL3'ün gördüğü cihazlar.
7. Xbox kontrolcüsünün SDL3 ile algılanması (model/bağlantı kaydı).
8. Linux i386 derlemesinde SDL3 sembollerinin dışa sızmadığı (CI'da `nm` testi).

## 7. Derleme

- **Windows:** GitHub Actions `windows-2022`, CMake + MSVC v143, `-A Win32`, `/MT` (upstream ile aynı).
- **Linux:** steamrt scout SDK i386 konteyneri, `g++-9`. Scout'ta SDL3 için gereken CMake sürümü bulunamazsa SDL3 aynı konteynerde elle yazılmış asgari derleme betiğiyle derlenir. Her durumda CI son `.so` dosyalarının istediği en yüksek `GLIBC_` sürümünü raporlar.
- **Testler:** `core` birim testleri her iki platformda; paket içerik testi (orijinal oyun verisi yok, beklenen dosyalar var).
