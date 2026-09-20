#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <WiFi.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>

#define PIN_DHT 4
#define PIN_SDA 21
#define PIN_SCL 22
#define PIN_ETH_MOSI 23
#define PIN_ETH_MISO 19
#define PIN_ETH_SCK 18
#define PIN_ETH_CS 5

#define OLED_ADDR 0x3C
#define OLED_LEBAR 128
#define OLED_TINGGI 64

#define WIFI_SSID "NAMA_WIFI"
#define WIFI_PASS "PASSWORD_WIFI"

#define INTERVAL_BACA 2000UL
#define INTERVAL_TAMPIL 1000UL
#define INTERVAL_KONEKSI 30000UL
#define DHCP_TIMEOUT 4000UL
#define DHCP_RESPON 1000UL
#define MAX_GAGAL 5
#define WDT_TIMEOUT_S 30

enum ModeKoneksi { MODE_NONE, MODE_ETH, MODE_WIFI };

DHT dht(PIN_DHT, DHT22);
Adafruit_SSD1306 oled(OLED_LEBAR, OLED_TINGGI, &Wire, -1);

byte mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00};

ModeKoneksi modeKoneksi = MODE_NONE;
float suhu = 0.0;
float kelembaban = 0.0;
bool sensorOk = false;
bool oledAktif = false;
bool wifiDimulai = false;
uint8_t gagalBaca = 0;

unsigned long tBaca = 0;
unsigned long tTampil = 0;
unsigned long tKoneksi = 0;

void mulaiWatchdog() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t cfg = {
    .timeout_ms = WDT_TIMEOUT_S * 1000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_reconfigure(&cfg);
#else
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);
}

bool mulaiEthernet() {
  Ethernet.init(PIN_ETH_CS);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) return false;
  if (Ethernet.linkStatus() == LinkOFF) return false;
  return Ethernet.begin(mac, DHCP_TIMEOUT, DHCP_RESPON) == 1;
}

void cekKoneksi(bool paksa) {
  if (modeKoneksi == MODE_ETH) {
    if (Ethernet.linkStatus() != LinkOFF) {
      Ethernet.maintain();
      return;
    }
    modeKoneksi = MODE_NONE;
  }

  if (modeKoneksi == MODE_WIFI && WiFi.status() != WL_CONNECTED) {
    modeKoneksi = MODE_NONE;
  }

  if (!paksa && millis() - tKoneksi < INTERVAL_KONEKSI) {
    if (modeKoneksi == MODE_NONE && WiFi.status() == WL_CONNECTED) {
      modeKoneksi = MODE_WIFI;
    }
    return;
  }
  tKoneksi = millis();

  if (mulaiEthernet()) {
    WiFi.disconnect(true);
    wifiDimulai = false;
    modeKoneksi = MODE_ETH;
    return;
  }

  if (!wifiDimulai) {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    wifiDimulai = true;
  }

  modeKoneksi = (WiFi.status() == WL_CONNECTED) ? MODE_WIFI : MODE_NONE;
}

void bacaDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    if (gagalBaca < MAX_GAGAL && ++gagalBaca >= MAX_GAGAL) {
      sensorOk = false;
    }
    return;
  }

  gagalBaca = 0;
  suhu = t;
  kelembaban = h;
  sensorOk = true;
}

void tampilkanOLED() {
  if (!oledAktif) return;

  char baris[24];
  const char* nama = "Tidak ada";
  String ip = "-";

  if (modeKoneksi == MODE_ETH) {
    nama = "Ethernet";
    ip = Ethernet.localIP().toString();
  } else if (modeKoneksi == MODE_WIFI) {
    nama = "WiFi";
    ip = WiFi.localIP().toString();
  }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);

  oled.setCursor(0, 0);
  snprintf(baris, sizeof(baris), "Jaringan: %s", nama);
  oled.print(baris);

  oled.setCursor(0, 10);
  oled.print(ip);

  oled.setTextSize(2);
  oled.setCursor(0, 28);
  if (sensorOk) {
    snprintf(baris, sizeof(baris), "Suhu:%.1fC", suhu);
  } else {
    snprintf(baris, sizeof(baris), "Suhu:ERR");
  }
  oled.print(baris);

  oled.setCursor(0, 48);
  if (sensorOk) {
    snprintf(baris, sizeof(baris), "Kelb:%.1f%%", kelembaban);
  } else {
    snprintf(baris, sizeof(baris), "Kelb:ERR");
  }
  oled.print(baris);

  oled.display();
}

void setup() {
  Serial.begin(115200);
  mulaiWatchdog();

  uint64_t id = ESP.getEfuseMac();
  for (uint8_t i = 1; i < 6; i++) {
    mac[i] = (id >> (8 * i)) & 0xFF;
  }

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  Wire.setTimeOut(100);
  oledAktif = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  dht.begin();

  SPI.begin(PIN_ETH_SCK, PIN_ETH_MISO, PIN_ETH_MOSI, PIN_ETH_CS);
  cekKoneksi(true);
  esp_task_wdt_reset();

  bacaDHT();
  tampilkanOLED();
}

void loop() {
  esp_task_wdt_reset();
  unsigned long sekarang = millis();

  if (sekarang - tBaca >= INTERVAL_BACA) {
    tBaca = sekarang;
    bacaDHT();
  }

  cekKoneksi(false);

  if (sekarang - tTampil >= INTERVAL_TAMPIL) {
    tTampil = sekarang;
    tampilkanOLED();
  }
}
