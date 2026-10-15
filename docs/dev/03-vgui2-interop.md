# VGUI2 Birlikte Çalışabilirlik Katmanı (Karar A)

Tarih: 2026-10-16

## Amaç

Ana menü, duraklatma menüsü, yükleme ekranları ve diyaloglar harita yüklü değilken de çizilmeli.
GoldSrc'de istemci DLL'inin bunu yapabildiği tek desteklenen yol, motorun istemciden sorguladığı
`VClientVGUI001` arayüzüdür. Motor bu arayüz üzerinden VGUI2 fabrikalarını ve bir ebeveyn paneli verir.
Biz de bu ebeveyne kendi kök panelimizi (VGUI2 `IClientPanel` uygulaması) bağlarız. vgui2, her karede
bu panelin `PaintTraverse` ve `Think` yöntemlerini çağırır.

## Lisans yaklaşımı

- Source SDK kodu yoktur. `bridge/include/lambda_bridge/vgui2_interop.h` kendi bildirimlerimizdir.
- Gerekli olan, motorun beklediği yöntem **sırasıdır** (vtable düzeni). Bu bir birlikte çalışabilirlik
  bilgisidir. Sıra, topluluğun GoldSrc tersine mühendislik notlarıyla karşılaştırılarak doğrulandı.
- Yalnızca kullandığımız yöntemlere kadar önekler bildirilir (IVGui, IPanel). `IClientPanel` vgui2
  tarafından çağrıldığı için eksiksiz uygulanır.
- Çizim, motorun GL bağlamında kendi OpenGL 1.x yardımcılarımızla yapılır (`bridge/src/gl.cpp`).
  GL fonksiyonları motorun SDL2'si üzerinden (`SDL_GL_GetProcAddress`) çözülür.

## Bilinen belirsizlikler (yalnızca oyun içinde doğrulanabilir)

| # | Soru | Prototipteki davranış | Kayıt |
|---|---|---|---|
| 1 | Motor, `UseVGUI1() == 1` iken de `Initialize`/`SetParent`/`Start` çağırıyor mu? | VGUI1 korunur (klasik modda özgün skor tablosu/menüler için) | `lambda.log`: `vgui2: Initialize`, `SetParent`, `Start` |
| 2 | Fabrika dizisinde hangi arayüzler var? | IVGui, IPanel, IEngineVGui, IBaseUI, IGameUI, IGameUIFuncs, IGameConsole aranır | `vgui2: interfaces ...` |
| 3 | Kök panelimiz GameUI'nin üstünde çiziliyor mu? | Motor kök paneline bağlanır, her `Think`'te öne alınır | Ekranda sağ üstte turuncu şerit |
| 4 | Menüde `PaintTraverse` çağrılıyor mu? | Çağrılırsa şerit menüde görünür | `vgui2: first PaintTraverse` |
| 5 | `IGameUI::IsGameUIActive` menü durumunu doğru veriyor mu? | Menü/oynanış bağlam seçimi buna dayanır | `GameUI active: 0/1` (debug) |
| 6 | GL durumu kaydet/geri yükle motorun çizimini bozuyor mu? | `glPushAttrib(GL_ALL_ATTRIB_BITS)` + matris yığını | Görsel kontrol |

Bu sorulardan herhangi biri olumsuz çıkarsa somut gözlem kaydedilir ve alternatif yol bu belgeye eklenir.
Belirsizlik giderilmeden menü yeniden tasarımı "tamamlandı" sayılmaz.

## Prototip test adımları (Windows, kullanıcı makinesi)

1. CI'dan `DemirarchLambdaUI-windows` paketini indirip içindeki `lambda_hl` klasörünü Half-Life klasörüne kopyala
   (orijinal dosyaların üzerine hiçbir şey yazılmaz).
2. Steam'i yeniden başlat; "Half-Life: Lambda UI" kütüphanede görünür. Ya da `hl.exe -game lambda_hl`.
3. Ana menüde sağ üst köşede turuncu şerit ve durum noktası görünmeli:
   gri = kontrolcü yok, sarı = bağlı ama seçilmedi, yeşil = aktif (A'ya basınca).
4. Konsolda `lambda_status`, `hlclassic`, `hlgamepad` denenir.
5. Oyun başlatılıp sol/sağ analog, tetikler ve A/B/X test edilir.
6. `lambda_hl/lambda/logs/lambda.log` dosyası incelenir.
