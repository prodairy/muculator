# Muculator Yol Haritası

Planlanan güncellemeler. Her madde bir GitHub Issue'ya dönüşebilir; bitenler `[x]` ile işaretlenir.

**Kaynak:** 🧑 = prodairy'nin önerisi · 🤖 = Claude'un önerisi
**Öncelik:** 🔴 hata / acil · 🟠 önemli · 🟢 olsa iyi olur

---

## v1.1: Düzeltmeler + küçük özellikler

### Hatalar
- [ ] 🔴 🧑 **Host Sync, Logic'te sessiz kanalda çalışmıyor.** Sebep bulundu: Logic, sesi olmayan ve seçili olmayan kanallardaki eklentileri çalıştırmıyor. Muculator tempoyu yalnızca çalışırken okuyabildiği için tempo gelmiyor.
  - **Yapılacak 1, uyarı:** Host Sync açıkken bir saniyeden uzun süre tempo gelmezse HOST TEMPO ekranında "IDLE" yazsın, son gelen tempo soluk gösterilsin. Alt satırda kısa bir ipucu çıksın: *"Logic sessiz kanallarda eklentiyi durdurur: kanalı seç, kayda hazırla (arm) ya da bölge olan bir kanalda Play'e bas."*
  - **Yapılacak 2, modül geçişi:** CHAIN kapalıyken başka modüle geçince host temposu o modüle de uygulansın. Şu an yalnızca tempo *değiştiğinde* uygulanıyor.
  - **Denenip vazgeçilen:** Eklentinin "sonsuz tail" bildirmesi Logic'i sürekli çalışmaya zorlardı, ama bounce sırasında "Include Audio Tail" seçeneğinin bitmemesine yol açabilir. Bu yüzden önerilmiyor.
  - Test: Logic'te boş kanal (seçili / seçili değil, Play / Stop), Ableton ve FL'de VST3.
- [ ] 🟠 🤖 **Sample rate alanları host'u takip etmiyor.** Converter ve Loop modülleri ilk açılıştaki değeri saklıyor; "HOST SR" butonuna basmadan güncellenmiyor.

### Pitch / Harmonics
- [ ] 🟠 🧑 **Odd / Even harmonik desteği.**
  - `ALL / ODD / EVEN` butonları.
  - Tabloya "tek / çift" sütunu.
  - Dalga şekli kısayolları: Square (yalnız tek harmonikler, genlik 1/n), Saw (hepsi, 1/n), Triangle (yalnız tek, 1/n²).
  - Harmonik sayısı seçilebilsin (16 / 32 / 64).
- [ ] 🟢 🤖 **Bemol yazım seçeneği:** C# yerine D♭ (# / ♭ butonu).
- [ ] 🟢 🤖 **Ayarlanabilir A4 referansı:** 440 varsayılan; 432 / 442 / 443 Hz ve elle giriş. Referans ekranda hep görünmeye devam eder.

### Kullanım kolaylığı
- [ ] 🟠 🧑 **Her modül için "Nasıl kullanılır?" butonu.** Başlıktaki küçük **?** butonuna basınca modülün üzerine bir yardım paneli açılsın:
  - Modül ne işe yarar (1–2 cümle).
  - Adım adım kullanım ve bir örnek; örneğin "120 BPM'lik A4 sample → 126 BPM → A#4'e yakın, mikroton uyarısı".
  - Terimlerin açıklaması: pre-delay, varispeed / time-stretch, cent, Q, harmonik.
  - Metinler önce İngilizce; Türkçe arayüz eklenince TR çevirisi.
- [ ] 🟠 🤖🧑 **Değeri tıklayınca panoya kopyala.** Tablodaki ms ya da Hz değerine tıkla, DAW'a yapıştır. Özellikle Harmonics tablosunda: frekansı EQ'ya ya da synth'e yapıştırmak için. Kopyalanınca hücre kısa bir süre yanıp "COPIED" yazsın.
- [ ] 🟢 🤖 **BPM ince ayar:** Shift + −/+ ile 0.1 adım; BPM ekranında fare tekerleği ve sürükleme.
- [ ] 🟢 🤖 **TAP için klavye kısayolu** (T tuşu).

---

## v1.2: Skin (tema) desteği

- [ ] 🟠 🧑 **Skin seçici.** DAW'lardan esinlenen ama marka adı kullanmayan temalar:

  | Skin adı | Esinlendiği hava | Karakter |
  |---|---|---|
  | **Console** | (şu anki) analog mikser | Koyu gri panel, LED ekran, yanan butonlar |
  | **Pattern** | step-sequencer DAW'ları | Mavi-gri gövde, turuncu/yeşil vurgular, parlak butonlar |
  | **Session** | minimal clip tabanlı DAW'lar | Düz (flat), açık gri, ince çizgiler, sarı/turuncu vurgu |
  | **Modular** | modüler / modern DAW'lar | Antrasit, tek turuncu vurgu, yuvarlak köşeler |
  | **Studio Pro** | klasik Mac DAW'ları | Grafit, mavi vurgu, yumuşak gradyan, cam efekti |

  - Teknik plan: renkler, köşe yarıçapı, font ve buton stili tek bir `Skin` yapısında toplanır. `ConsoleLookAndFeel` bu yapıdan okur. Skin seçimi DAW projesiyle birlikte kaydedilir.
  - Her skin için ekran görüntüsü alınıp README'ye eklenir.
- [ ] 🟢 🤖 **Açık / koyu mod** (skin'den bağımsız).
- [ ] 🟢 🤖 **Yeniden boyutlandırılabilir pencere:** %75 / %100 / %125 / %150 ölçek.

---

## v1.3: CLAP formatı

- [ ] 🟠 🧑🤖 **CLAP desteği.** Açık ve ücretsiz eklenti formatı; Bitwig, REAPER, FL Studio ve diğerleri destekliyor. Açık kaynak bir proje için önemli.
  - JUCE 9.0.2'nin kendisinde CLAP yok. Bunun yerine açık kaynak **clap-juce-extensions** (MIT lisanslı) ile CMake'e `CLAP` hedefi eklenecek. Önce JUCE 9 ile uyumlu olup olmadığı denenecek.
  - GitHub'daki otomatik derlemeye CLAP zip dosyaları eklenecek (Windows + macOS).
  - CLAP gelince **Linux sürümü** de aynı yoldan eklenebilir (VST3 + CLAP).

## v1.4: Yeni hesaplar

- [ ] 🟠 🤖 **Loop → BPM (ters hesap).** Sample'ın uzunluğunu (saniye ya da sample) ve bar sayısını gir, BPM'i bul. Sample paketlerindeki loop'lar için çok işe yarar.
- [ ] 🟠 🤖🧑 **Stretch'e "time-stretch" (perde sabit) modu.** Şu an yalnızca varispeed var: hız değişince perde de değişir, kaset gibi. Yeni modda perde sabit kalır (Warp / Flex Time mantığı). Gösterilecekler: stretch oranı ve yüzdesi, yeni sample uzunluğu ve oran çok uç olursa kalite uyarısı. İki mod tek butonla değişsin: `VARISPEED / TIME-STRETCH`. Modun ne olduğunu "Nasıl kullanılır?" panelinde de açıkla.
- [ ] 🟢 🤖 **Stretch'te "gama oturt":** Seçilen ton/gamdaki (örneğin A minör) notalara oturan en yakın BPM'lerin listesi.
- [ ] 🟢 🤖 **Delay'e swing:** %50–75 swing için 1/8 ve 1/16 süreleri; Haas efekti için 10–35 ms önerisi.
- [ ] 🟢 🤖 **Dosya boyutu hesabı:** Süre + sample rate + bit derinliği + kanal sayısı → MB.
- [ ] 🟢 🤖 **Enstrüman frekans haritası:** Kick, bas, vokal vb. temel ve harmonik bölgeleri (EQ rehberi).

---

## Altyapı / dağıtım

- [ ] 🟠 🤖 **CI'da ekran görüntüsü testi.** Snapshot aracı GitHub'da çalışsın, arayüz bozulursa yakalansın.
- [ ] 🟠 🤖 **pluginval testi.** VST3 için otomatik uyumluluk testi; `auval`'ın Windows'taki karşılığı.
- [ ] 🟢 🤖 **macOS imzalama + notarization.** "Engellendi" uyarısı kalkar; Apple Developer hesabı gerekir (yılda 99 $).
- [ ] 🟢 🤖 **README'yi her sürümde güncelleyen sürüm notları** (CHANGELOG.md).

---

## Fikir kutusu (henüz karar verilmedi)

- Türkçe arayüz seçeneği (TR / EN).
- Favori değerleri kaydetme (preset).
- Mikroton eşiğini ayarlanabilir yapmak (5 / 10 / 15 cent).
