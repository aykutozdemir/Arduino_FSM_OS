# FsmOS Library Documentation

Bu dizin FsmOS kütüphanesinin bağımsız Doxygen dokümantasyonunu içerir.

## FsmOS Hakkında

FsmOS, Arduino için hafif ve kooperatif bir görev zamanlayıcısıdır. Çoklu görev işleme, mesaj geçişi ve sistem izleme özellikleri sunar.

### Özellikler

- **Kooperatif Çoklu Görev**: Öncelik kesintisi olmadan çoklu görev çalıştırma
- **Mesaj Geçişi**: Görevler arası iletişim (publish/subscribe)
- **Bellek Verimli**: AVR mikrodenetleyiciler için optimize edilmiş
- **Hata Ayıklama Desteği**: Yerleşik loglama ve tanılama
- **Görev Bütçeleme**: Mesaj kuyruğu taşmalarını önleme
- **Bellek İzleme**: RAM, yığın, heap, flash ve EEPROM kullanım takibi

## Dokümantasyon Oluşturma

### Gereksinimler
- Doxygen 1.9.8 veya üzeri
- Linux/Unix ortamı

### Komutlar

```bash
# FsmOS dizinine git
cd lib/FsmOS

# Dokümantasyon oluştur
doxygen Doxyfile

# Dokümantasyonu temizle
rm -rf docs/
```

### Çıktı
Dokümantasyon `docs/html/` dizininde HTML formatında oluşturulur.

Ana sayfa: `docs/html/index.html`

## Doxyfile Konfigürasyonu

Bu Doxyfile FsmOS kütüphanesi için özel olarak optimize edilmiştir:

### Proje Bilgileri
- **Proje Adı**: FsmOS
- **Versiyon**: 1.3.0
- **Açıklama**: A lightweight cooperative task scheduler for Arduino
- **Yazar**: Aykut Özdemir <aykutozdemirgyte@gmail.com>

### Çıktı Ayarları
- **HTML**: Aktif (ana çıktı formatı)
- **LaTeX**: Pasif
- **RTF**: Pasif
- **Man Pages**: Pasif
- **XML**: Pasif

### Dokümantasyon Ayarları
- **EXTRACT_ALL**: Tüm sembolleri dokümante et
- **EXTRACT_PRIVATE**: Private üyeleri dahil et
- **EXTRACT_STATIC**: Static üyeleri dahil et
- **HIDE_UNDOC_MEMBERS**: Dokümante edilmemiş üyeleri gizle
- **CASE_SENSE_NAMES**: Büyük/küçük harf duyarlılığı

### Grafik Ayarları
- **CLASS_GRAPH**: Sınıf diyagramları
- **COLLABORATION_GRAPH**: İşbirliği diyagramları
- **INCLUDE_GRAPH**: Include diyagramları
- **CALL_GRAPH**: Çağrı diyagramları (pasif)

### Giriş Dosyaları
- `FsmOS.h` - Ana header dosyası
- `FsmOS.cpp` - Ana implementation dosyası
- `examples/` - Örnek kodlar
- `README.md` - Ana sayfa olarak kullanılır

## Dokümantasyon İçeriği

### Ana Sınıflar
- **Task**: Temel görev sınıfı
- **Scheduler**: Görev zamanlayıcısı
- **SharedMsg**: Paylaşımlı mesaj sınıfı
- **MsgDataPool**: Mesaj veri havuzu
- **LinkedQueue**: Bağlantılı kuyruk
- **Mutex**: Karşılıklı dışlama
- **Semaphore**: Semafor

### Veri Yapıları
- **TaskNode**: Görev düğümü
- **TaskStats**: Görev istatistikleri
- **SystemMemoryInfo**: Sistem bellek bilgisi
- **ResetInfo**: Sıfırlama bilgisi
- **MemoryStats**: Bellek istatistikleri

### Enum'lar
- **Priority**: Görev öncelik seviyeleri
- **State**: Görev durumları
- **LogLevel**: Log seviyeleri
- **ResetCause**: Sıfırlama nedenleri

## Örnek Kodlar

Kütüphane 13 farklı örnek içerir:

1. **BasicBlink**: Basit LED yanıp sönme
2. **ButtonLed**: Görevler arası iletişim
3. **Diagnostics**: Sistem izleme ve hata ayıklama
4. **DynamicTasks**: Çalışma zamanında görev oluşturma/silme
5. **Logger**: Yerleşik loglama sistemi
6. **MemoryMonitoring**: Kapsamlı bellek kullanım takibi
7. **MemoryOptimization**: Bellek verimli kodlama
8. **MemoryOptimizedTimers**: Zamanlayıcı kullanım optimizasyonu
9. **MessageQueueing**: Görev askıya alma sırasında mesaj işleme
10. **MutexExample**: Karşılıklı dışlama senkronizasyonu
11. **SemaphoreExample**: Semafor tabanlı senkronizasyon
12. **TaskLifecycle**: Görev durum yönetimi
13. **TaskNames**: İsimli görevler ve durum takibi
14. **TaskTimingMonitoring**: Görev zamanlama izleme

## Özellikler

### Dokümantasyon Kalitesi
- ✅ Tüm public sınıflar dokümante edilmiş
- ✅ Tüm public metodlar dokümante edilmiş
- ✅ Parametre açıklamaları mevcut
- ✅ Dönüş değeri açıklamaları mevcut
- ✅ Örnek kodlar dahil edilmiş
- ✅ Sınıf diyagramları oluşturulmuş

### Navigasyon
- **Ana Sayfa**: Kütüphane genel bakış
- **Sınıflar**: Tüm sınıfların listesi
- **Dosyalar**: Kaynak dosyalar
- **Fonksiyonlar**: Alfabetik fonksiyon listesi
- **Modüller**: Grup dokümantasyonu
- **Örnekler**: Örnek kodlar

## Kurulum

### Arduino IDE
1. Arduino IDE'yi açın
2. `Sketch > Include Library > Manage Libraries...` menüsüne gidin
3. "FsmOS" arayın
4. Install'e tıklayın

### Manuel Kurulum
1. Bu repository'yi indirin
2. `FsmOS` klasörünü Arduino kütüphaneleri dizinine kopyalayın
3. Arduino IDE'yi yeniden başlatın

## Kullanım

```cpp
#include <FsmOS.h>

// Basit bir yanıp sönen görev tanımla
class BlinkTask : public Task
{
public:
    BlinkTask() : Task(F("Blinker"))
    {
        set_period(500);  // Her 500ms'de çalıştır
    }

    uint8_t getMaxMessageBudget() const override { return 0; }
    uint16_t getTaskStructSize() const override { return sizeof(*this); }

    void on_start() override
    {
        pinMode(LED_BUILTIN, OUTPUT);
        log_info(F("Blink task started"));
    }

    void step() override
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
};

BlinkTask blinker;

void setup()
{
    Serial.begin(9600);
    OS.begin_with_logger();  // Loglama ile başlat
    OS.add(&blinker);
}

void loop()
{
    OS.loop_once();
}
```

## Sorun Giderme

### Uyarılar
Dokümantasyon oluşturulurken bazı uyarılar görülebilir:
- Obsolete tag uyarıları (normal)
- Undocumented symbol uyarıları (kontrol edilmeli)

### Performans
- Dokümantasyon oluşturma süresi: ~3-5 saniye
- Çıktı boyutu: ~1-3 MB
- Dosya sayısı: ~150+ HTML dosyası

## Güncelleme

Kod değişikliklerinden sonra dokümantasyonu yeniden oluşturun:

```bash
# Temizle ve yeniden oluştur
rm -rf docs/
doxygen Doxyfile
```

## Lisans

Bu kütüphane MIT lisansı altındadır.
Copyright 2025 Aykut Özdemir <aykutozdemirgyte@gmail.com>

## Katkıda Bulunma

1. Repository'yi fork edin
2. Feature branch oluşturun
3. Değişikliklerinizi commit edin
4. Branch'inizi push edin
5. Pull Request oluşturun
