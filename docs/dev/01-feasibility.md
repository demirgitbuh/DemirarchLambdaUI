# Demirarch Lambda UI — Aşama 1: Teknik ve Hukuki Uygulanabilirlik İncelemesi

Tarih: 2026-10-16
Durum: **İnceleme tamamlandı. 4 karar 2026-10-16'da kullanıcı tarafından verildi** (bölüm 9):
1. Çok oyunculu yalnızca Lambda UI sunucularıyla → **kabul**.
2. VGUI2 → **A: kendi birlikte çalışabilirlik katmanımız** (Source SDK kodu yok).
3. OF/BS → **TWHL Updated referans uygulamaları taban**.
4. **Herkese açık depo** `demirgitbuh/DemirarchLambdaUI` + GitHub Actions; commit'lerde e-posta yok.
Bu belge geliştirme notudur (Türkçe). Kullanıcıya dönük tüm UI metinleri İngilizce olacaktır.

---

## 1. Çalışma alanı ve araç zinciri (yerel makine)

| Öğe | Bulgu | Sonuç |
|---|---|---|
| İşletim sistemi | Windows 11 Pro 10.0.26200 | Windows hedefi için geliştirme makinesi |
| Git / gh | `git` var, `gh` hesabı `demirgitbuh` | GitHub Actions ile derleme mümkün |
| MSVC / CMake / Ninja | **Yok** (`cl`, `cmake`, `msbuild`, vswhere sonucu boş) | Yerel derleme yok → derleme GitHub Actions'ta (windows-2022 + MSVC v143 Win32, Linux i386) |
| Python | 3.12 | İkili inceleme ve test yardımcıları için kullanılabilir |
| Linux makinesi | Yok | Linux/SteamOS fiziksel doğrulaması şu an yapılamaz |
| Kontrolcü | Yalnızca bir Xbox kontrolcüsü (model bilinmiyor) | PlayStation/Steam Deck fiziksel testi yapılamaz |

## 2. Hedef oyun kurulumu (doğrulandı)

Steam kütüphanesi: `C:\Program Files (x86)\Steam\steamapps\common\Half-Life`

| Uygulama | appid | Durum |
|---|---|---|
| Half-Life | 70 | Kurulu, **buildid 15961492** (dosyalar 2026-04-08), varsayılan dal |
| Opposing Force | 50 | Kurulu (`gearbox/`) |
| Blue Shift | 130 | Kurulu (`bshift/`) |
| Half-Life Legacy | 3619040 | Ayrı uygulama olarak kurulu — **hedef değil, kullanılmayacak** |

### Motor ikilileri (PE başlığından okundu)

| Dosya | Mimari | Derleme damgası | Not |
|---|---|---|---|
| `hw.dll` | x86 (0x14c, 32-bit) | 2024-10-08 | OpenGL motoru |
| `hl.exe` | x86 | 2024-10-08 | |
| `SDL2.dll` | x86 | — | Motorun kendi SDL2'si |
| `valve/cl_dlls/client.dll` | x86 | 2024-10-08 | |
| `gearbox/dlls/opfor.dll` | x86 | 2020-08-19 | Kaynak kodu yayımlanmamış |
| `bshift/dlls/hl.dll` | x86 | 2020-08-04 | Kaynak kodu yayımlanmamış |

**Sonuç:** Tüm mod ikilileri 32-bit x86 olmalı (Windows: Win32 DLL; Linux: i386 `.so`). 64-bit derleme motora yüklenemez.

### Motorun kontrolcü/girdi ile ilişkisi (import tablosundan doğrulandı)

- `hw.dll` SDL2'den yalnızca pencere, GL, fare, klavye ve olay fonksiyonlarını içe aktarıyor (`SDL_CreateWindow`, `SDL_PollEvent`, `SDL_StartTextInput`, `SDL_GetRelativeMouseState`, …). **`SDL_GameController*` fonksiyonlarının hiçbirini kullanmıyor.**
- `valve/cl_dlls/client.dll` ise `SDL_GameControllerOpen/GetAxis/GetButton/Close`, `SDL_NumJoysticks`, `SDL_IsGameController` kullanıyor. `gearbox/cl_dlls/client.dll` de aynı.
- **Sonuç:** GoldSrc'de kontrolcü girdisinin tamamı `client.dll` içinde işleniyor. Kendi `client.dll`'imizi taşıyan ayrı bir mod, kontrolcü katmanını motora dokunmadan tamamen devralabilir. Bu, projenin temel taşıdır.
- `hw.dll` içinde `SteamInput006` arayüz adı geçiyor. Motorun ISteamInput'u nasıl kullandığı (eylem setleri mi, yalnızca başlatma mı) statik olarak belirlenemedi → **Aşama 3 prototipinde çalışma zamanında doğrulanacak** (Steam Input çakışma rehberini etkiler).

### Motorun dışa açtığı arayüzler (`hw.dll` dizgileri)

`VClientVGUI001`, `BaseUI001`, `GameUI007`, `VEngineVGui001`, `VGUI_Panel007`, `VGUI_Surface026`, `VGUI_Input004`, `VGUI_ivgui006`, `VGUI_Scheme009`, `VGUI_Localize003`, `VGUI_System009`, `GameConsole003`, `VENGINE_GAMEUIFUNCS_VERSION001`, `VFileSystem009`, `KeyValues003`, `SteamUtils010`, `SteamInput006` …

- Motor, `client.dll`'den **`VClientVGUI001`** (IClientVGUI) arayüzünü sorguluyor. Bu, istemcinin bir VGUI2 kök paneli alıp **harita yüklü değilken de (ana menüde)** çizim ve güncelleme yapabilmesinin bilinen tek desteklenen yoludur. CS 1.6/CZ ve BugfixedHL bu yolu kullanır.
- Ana menüyü çizen `GameUI.dll` (`valve/cl_dlls/GameUI.dll`, `GameUI007`) kapalı kaynaktır. Kodu değiştirilemez; yalnızca kaynak dosyaları (`resource/GameMenu.res` vb.) mod tarafından geçersiz kılınabilir.
- Sunucu tarayıcısı `platform/servers/ServerBrowser.dll` (kapalı kaynak, fare odaklı).

## 3. SDK ve oyun kodu kaynakları

| Kaynak | Lisans | Durum | Kullanım |
|---|---|---|---|
| [ValveSoftware/halflife](https://github.com/ValveSoftware/halflife) | Half Life 1 SDK License | Son itme 2024-10-02 (HL25 SDK) | Resmî referans |
| [twhl-community/halflife-updated](https://github.com/twhl-community/halflife-updated) | Half Life 1 SDK License | Aktif (2026-10-10) | **HL entegrasyonu için taban** — "Only the latest Steam version of Half-Life is supported" |
| [twhl-community/halflife-op4-updated](https://github.com/twhl-community/halflife-op4-updated) | Half Life 1 SDK License | 2025-10-05 | OF için tek erişilebilir oyun kodu — "reference implementation" |
| [twhl-community/halflife-bs-updated](https://github.com/twhl-community/halflife-bs-updated) | Half Life 1 SDK License | 2024-09-01 | BS için tek erişilebilir oyun kodu — "reference implementation" |
| [tmp64/BugfixedHL-Rebased](https://github.com/tmp64/BugfixedHL-Rebased) | **GPL-3.0** | Aktif | **Kod alınmayacak** (aşağıya bakın) |
| [tmp64/hl1_source_sdk](https://github.com/tmp64/hl1_source_sdk) | **Source 1 SDK License** | — | VGUI2/tier1 başlıkları — lisans kapsamı sorunlu (bölüm 4.2) |
| [twhl-community/halflife-unified-sdk](https://github.com/twhl-community/halflife-unified-sdk) | HL SDK + çeşitli | Aktif | VGUI2 kullanmıyor; referans olarak incelendi |

Gearbox, Opposing Force ve Blue Shift oyun kodunu yayımlamadı. Updated depoları bu oyunların davranışını HL SDK üzerinde yeniden kuran topluluk referans uygulamalarıdır; çalışmak için oyunların kullanıcıda kurulu olması gerekir (içerik `gearbox/` ve `bshift/` dizinlerinden gelir, pakete dahil edilmez).

## 4. Lisans incelemesi

### 4.1 Half Life 1 SDK License (metin doğrudan okundu)

- Değiştirilmiş oyunu kaynak ve nesne kodu olarak dağıtmaya izin verir, **"but only for free"**.
- Dağıtımda LICENSE dosyası ve uyarı metni bulunmalı.
- **Sonuç:** SDK'dan türetilen kod (server/client oyun kodu) OSI anlamında "açık kaynak" lisansa (MIT, GPL) **çevrilemez**. Kaynak açık yayımlanabilir, ancak HL SDK lisansı altında kalır.
- **GPL-3.0 ile uyumsuz:** "only for free" ek kısıtı GPL'in ek kısıt yasağıyla çelişir. Bu yüzden BugfixedHL-Rebased'den **kod kopyalanmayacak**.

### 4.2 VGUI2 başlıkları sorunu (karar gerekiyor)

IClientVGUI ve VGUI2 arayüzleri resmî HL SDK'da **yok**. Topluluk kullanımındaki tek hazır kaynak `hl1_source_sdk`, Source SDK 2013 parçalarıdır. Source 1 SDK lisansı izni şöyle tanımlar: *"to develop a modified Valve game running on the Source engine"*. GoldSrc modunda kullanmak bu iznin lafzının **dışında** kalır. Yaygın topluluk pratiği olsa da yazılı izin yoktur.

Seçenekler:

- **A — Kendi asgari birlikte çalışabilirlik katmanımız (önerilen):** Yalnızca gereken birkaç arayüzün (IClientVGUI, IClientPanel, IEngineVGui, IBaseUI, surface/input'tan gereken kısım) vtable düzenini kendi başlıklarımızla, kendi kodumuz olarak tanımlarız. UI'nin kendisini (odak, liste, klavye, çark, HUD) zaten gamepad için sıfırdan yazmamız gerekiyor. Bu yüzden fare odaklı `vgui_controls`'a ihtiyaç yok. Tek bir kök panel içinde kendi OpenGL/TriAPI çizicimizle çizeriz. Klavye/fare girdisi motorun zaten yüklü SDL2'sine `SDL_AddEventWatch` ile (zlib, açık API) gözlemci olarak bağlanarak alınır.
  - Artı: Source SDK kodu yok, lisans durumu temiz.
  - Eksi: vtable düzenlerinin doğruluğu prototipte kanıtlanmalı; ilk iş yükü daha büyük.
- **B — `hl1_source_sdk` kullanmak:** Daha hızlı, kanıtlanmış yol (BugfixedHL çalışıyor). Lisans kapsamı belirsiz; README'de açıkça belirtilir.
- **C — Ana menüyü yeniden tasarlamamak:** Şartnameye aykırı, önerilmez.

### 4.3 Üçüncü taraf bağımlılıklar (asgari set)

| Bağımlılık | Sürüm adayı | Lisans | Gerekçe | Mimari |
|---|---|---|---|---|
| SDL3 | 3.4.16 (3.4.18 çok yeni, 2026-10-02) | zlib | Kontrolcü algılama, hot-plug, gyro, touchpad, LED, tetik efektleri (`SDL_SendGamepadEffect`), pil (`SDL_GetGamepadPowerInfo`), Steam sanal gamepad kimliği | x86 Win32 ve i386 Linux için statik derlenir |
| miniaudio | 0.11.25 | Public domain / MIT-0 | **Yalnızca** DualSense USB ses-tabanlı gelişmiş haptik kanalları için (SDL3 bu haptikleri desteklemiyor) | Tek başlık, statik |
| Steam flat API | Oyunun kendi `steam_api.dll`'i (SteamUtils010) | — (Steamworks SDK başlıkları **depoya konmaz**; yalnızca gereken birkaç düz C fonksiyonu çalışma zamanında `GetProcAddress` ile çözülür) | Steam/Deck ekran klavyesi (`ShowGamepadTextInput` / `ShowFloatingGamepadTextInput`) | x86 |

Önerilen lisans düzeni: SDK'dan türetilen oyun kodu → **Half Life 1 SDK License**; SDK'dan bağımsız özgün çekirdek (`lambda_core`: input, profil, UI çatısı, nişan matematiği) → **MIT**; SDL3 → zlib bildirimi; miniaudio → MIT-0 bildirimi. `THIRD_PARTY_NOTICES.md` ayrı tutulur.

## 5. Özellik bazında uygulanabilirlik

| Alan | Uygulama yolu | Durum |
|---|---|---|
| Kontrolcü girdisi | Kendi `client.dll`'imizde SDL3 (statik, gizli semboller). Motorun SDL2'si kontrolcü açmıyor → çakışma yok. Klasik modda SDL3 alt sistemi tamamen kapatılır ve SDK'nın özgün SDL2 joystick yolu geri gelir | Uygulanabilir |
| SDL2/SDL3 bir arada | Windows: SDL3 statik → mod dizini dışına DLL koymaya gerek yok. Linux: `-fvisibility=hidden` + `--exclude-libs,ALL`, aksi halde `SDL_*` sembolleri motorun `libSDL2`'sine bağlanır (gerçek bir çökme riski) | Uygulanabilir, CI'da sembol tablosu testiyle doğrulanacak |
| Ana menü / tüm menüler | IClientVGUI kök paneli üzerinde tam ekran kendi UI'miz; GameUI `IBaseUI` ile gizlenir/gösterilir | **Prototip riski:** GameUI z-sırası ve ESC davranışı çalışma zamanında doğrulanmalı |
| Sunucu tarayıcısı | Kapalı `ServerBrowser.dll` yerine kendi tarayıcımız: Steam master sunucu sorgusu + A2S bilgi sorguları (açık protokol) | Uygulanabilir, iş yükü yüksek |
| Konsol | Kendi konsol panelimiz; komut gönderme `ClientCmd`, çıktı için `GameConsole003`/konsol yazdırma kancası | Çıktı yakalama yolu prototipte doğrulanacak |
| Yeni HUD | `HUD_Redraw` + TriAPI/OpenGL | Uygulanabilir |
| Tek oyunculuda duraklatma | `pause` / GameUI etkinleştirme | Uygulanabilir |
| Tek oyunculuda yavaşlatma | Motorda `sys_timescale` (varsayılan 1.0) var; cheat bayrağı statik olarak okunamadı | **Çalışma zamanında doğrulanacak.** Cheat korumalıysa yavaşlatma seçeneği "teknik engel" olarak raporlanıp onay istenecek |
| Nişan yardımı | Server DLL bizim → `UTIL_TraceLine` ile görüş hattı, `pev->takedamage`/`IsAlive`/sınıf bilgisiyle hedef geçerliliği; kamera yavaşlama/takip istemcide, mermi yönü yardımı sunucuda (SDK'daki `GetAutoaimVector` altyapısı üzerine) | Uygulanabilir |
| Silah çarkı | İstemci silah listesi (`WEAPON` mesajları, `gWR`) + `ClientCmd("weapon_xxx")` | Uygulanabilir |
| Kayıt/yükleme | `SAVE/*.sav` dosyaları + motorun yazdığı `.tga`/metadata; `save`/`load` komutları | Uygulanabilir; metadata eksikse dürüst gösterim |
| Ekran klavyesi | Yerleşik klavye her zaman; Steam/Deck klavyesi flat API ile isteğe bağlı | Uygulanabilir |
| Jiroskop / touchpad / LED / pil | SDL3 sensör, touchpad, LED, güç API'leri | Uygulanabilir (cihaz bağımlı) |
| Uyarlanabilir tetikler | SDL3 `SDL_SendGamepadEffect` ile DS5 efekt paketi (USB ve BT) | Uygulanabilir, fiziksel test yok |
| DualSense gelişmiş haptik | USB ses aygıtının 3./4. kanalı (miniaudio). **Bluetooth'ta mümkün değil** → kilitli + açıklama | Uygulanabilir (yalnız USB), fiziksel test yok |
| Steam Deck | Linux i386 derleme + Steam Input düzeni; arka tuşlar yalnızca Steam Input eşlemesiyle | Uygulanabilir, donanım yok |
| OF ve BS | `halflife-op4-updated` / `halflife-bs-updated` tabanlı ayrı mod dizinleri, `fallback_dir` ile `gearbox`/`bshift` içeriği | Uygulanabilir (bölüm 9, karar 3) |
| Güvenli başlatma | `-lambdasafe` başlatma parametresi (`gEngfuncs.CheckParm`) + kurtarma kombinasyonu | Gerçek sözdizimi Aşama 3'te doğrulanıp belgelenecek |

## 6. Çok oyunculu: yapısal kısıt (karar gerekiyor)

Şartname **ayrı mod** ve **orijinal dosyaların üzerine yazmama** istiyor. GoldSrc'de istemci ile sunucunun oyun dizini (gamedir) aynı olmalı. Updated README'si de bunu söylüyor: *"You cannot use Updated clients to play on vanilla servers"*.

- Lambda UI modu (`lambda_hl`) yalnızca **aynı modu çalıştıran sunuculara** (dinleme sunucusu, LAN, mod sunucusu kurulmuş HLDS) bağlanabilir.
- Vanilla `valve` sunucusuna bağlanmak motorun oyunu `valve` dizinine geçirmesine yol açar ve arayüzümüz devreden çıkar.
- BugfixedHL'in vanilla sunucularda çalışmasının nedeni `valve/cl_dlls/client.dll`'in **üzerine yazılmasıdır**. Bu, şartname gereği yasaktır.
- Blue Shift'in çok oyunculu modu yoktur (`type "singleplayer_only"`). Opposing Force çok oyunculu modu OF modu üzerinden desteklenir.

Sonuç: Çok oyunculu ekranlar, sohbet, bas-konuş, skor tablosu, hazır mesaj çarkı ve VAC uyarısı tam uygulanır; ancak **erişilebilir sunucular Lambda UI sunucularıyla sınırlıdır**. Bu, şartnamenin kendi koşullarından doğan teknik bir sonuçtur ve kullanıcı onayı gerektirir.

## 7. Platform notları

- **Windows 11:** MSVC v143, Win32 hedefi, statik CRT değil (motor VC140 kullanıyor; `/MD` ile uyumlu).
- **Linux LTS:** Hedef olarak **Ubuntu 24.04 LTS** öneriliyor. HL25 ile yerli Linux sürümü varsayılan oldu. İkililer GCC ile `-m32`, `-static-libstdc++ -static-libgcc` derlenecek. glibc tabanı, Steam Runtime SDK konteynerinde derlenerek sabitlenecek (scout/sniper seçimi Aşama 3'te CI'da doğrulanacak).
- **SteamOS (Steam Deck):** Aynı Linux i386 ikilisi. Kararlı SteamOS sürümü, test donanımı sağlandığında kayda geçirilecek.
- **Xbox Kablosuz Adaptör:** Windows'ta yerleşik. Linux/SteamOS'ta çekirdek ağacı dışı `xone` sürücüsü gerekir ([medusalix/xone](https://github.com/medusalix/xone) son itme 2025-12, aktif çatal [dlundqvist/xone](https://github.com/dlundqvist/xone) 2026-03). Bu bir donanım/sürücü kısıtıdır, mod tarafından çözülemez.
- **DualShock 3 / Windows:** Üçüncü taraf sürücü gerekir ([nefarius/DsHidMini](https://github.com/nefarius/DsHidMini), BSD-3). Linux'ta `hid-sony` çekirdek sürücüsü. BT eşleşmesi için ek araç gerekir.
- **Xbox Elite arka tuşları:** Windows'ta XInput arka tuşları göstermez. SDL3'ün WGI arka ucu (`SDL_windows_gaming_input.c`) veya GameInput ile erişilebilir. Bluetooth'ta HIDAPI. Bağlantıya göre ayrı ayrı doğrulanacak.

## 8. Doğrulama sınırları (dürüst durum)

- Şu an yalnızca bir Xbox kontrolcüsü var. PlayStation ailesi, DualSense Edge, Steam Deck ve Linux makinesi yok.
- Şartnamenin 20. ve 23. bölümleri gereği bu donanımlar olmadan **v0.1.0 "tamamlandı" ilan edilemez**. Kod yazılabilir ve "Uygulandı, fiziksel test bekliyor" olarak işaretlenir. Tamamlanma için fiziksel test erişimi gerekir.
- Yerel derleme ortamı yok; tüm derleme ve otomatik testler GitHub Actions'ta çalışacak. Oyun içi testler kullanıcının makinesinde yapılacak.

## 9. Kullanıcı kararı gereken konular

1. **Çok oyunculu kapsamı:** Erişilebilir sunucuları Lambda UI sunucularıyla sınırlamak (bölüm 6).
2. **VGUI2 entegrasyon yolu:** A (kendi birlikte çalışabilirlik katmanı, önerilen) veya B (`hl1_source_sdk`, Source SDK lisans kapsamı belirsiz) (bölüm 4.2).
3. **OF/BS tabanı:** TWHL Updated referans uygulamalarını HL SDK lisansıyla taban almak (tek erişilebilir yol).
4. **GitHub deposu ve CI:** `demirgitbuh/DemirarchLambdaUI` herkese açık depo + GitHub Actions. Commit'lerde e-posta yok (`user.email` boş).

## 10. Sonraki adım (onaydan sonra): Aşama 2–3

- Destek matrisi (`docs/support-matrix.md`) ve entegrasyon planı.
- Depo iskeleti: `lambda_core` (MIT, motordan bağımsız, birim testli), `games/hl`, `games/opfor`, `games/bshift` (HL SDK lisansı), SDL3 statik derleme, CI (Win32 + Linux i386).
- **Prototip doğrulamaları (oyun içinde, kullanıcı makinesinde):** IClientVGUI kök panelinin menüde çizilmesi, GameUI gizleme/z-sırası, `sys_timescale` davranışı, `-lambdasafe` parametresinin iletilmesi, motorun SteamInput kullanımı, Xbox kontrolcüsünün SDL3 ile algılanması.
