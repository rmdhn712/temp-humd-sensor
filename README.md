# Sensor Suhu dan Kelembapan

# 🌡️ Monitoring Suhu & Kelembaban ESP32 (Multi-Network & OLED Display)

Dokumentasi ini menjelaskan fungsionalitas, arsitektur, dan cara kerja dari sistem embedded berbasis **ESP32** yang dirancang untuk mengukur kondisi lingkungan menggunakan sensor **DHT22**, menampilkannya pada layar **OLED SSD1306**, serta memiliki fitur manajemen jaringan otomatis (*failover*) antara **Ethernet W5500** dan **WiFi**.

---

## 🚀 Fitur Utama
* **Dual-Network Autoshifting (Failover):** Memprioritaskan koneksi kabel **Ethernet** demi stabilitas data. Sistem secara otomatis beralih ke **WiFi** apabila kabel Ethernet terputus atau gagal mendapatkan IP DHCP, dan kembali ke Ethernet jika jaringan pulih.
* **Smart Fault-Handling Sensor:** Sistem memiliki toleransi pembacaan hingga 5 kali kegagalan beruntun sebelum menyatakan status sensor rusak atau *Error*.
* **Hardware Watchdog Timer (WDT):** Mencegah sistem dari kondisi *crash* atau *hang* dengan melakukan *reboot* otomatis jika sistem tidak merespon dalam waktu 30 detik.
* **Real-time Interface:** Status koneksi jaringan, alamat IP, serta data suhu dan kelembaban diperbarui secara berkala dan ditampilkan langsung di layar OLED.

---

## 🛠️ Konfigurasi Pin Out (Wiring)

Berikut adalah mapping pin GPIO pada **ESP32** untuk menghubungkan seluruh modul peripheral:

### 1. Sensor DHT22
* **DATA Pin** ➡️ GPIO 4

### 2. Layar OLED I2C (SSD1306)
* **SDA Pin** ➡️ GPIO 21
* **SCL Pin** ➡️ GPIO 22
* *Alamat I2C: `0x3C`*

### 3. Modul Ethernet (SPI)
* **MOSI Pin** ➡️ GPIO 23
* **MISO Pin** ➡️ GPIO 19
* **SCK Pin** ➡️ GPIO 18
* **CS Pin**   ➡️ GPIO 5
* *MAC Address di-generate secara otomatis berbasis eFuse MAC bawaan chip ESP32.*

---

## ⚙️ Cara Kerja Kode Program

### 📑 1. Manajemen Jaringan (`cekKoneksi`)
Fungsi ini berjalan secara non-blocking dengan interval pengecekan setiap 30 detik.
* **Ethernet Priority:** Sistem mendeteksi status hardware dan sambungan kabel. Jika aktif, fungsi DHCP akan dijalankan.
* **WiFi Fallback:** Jika Ethernet tidak tersedia, WiFi dengan mode Station (`WIFI_STA`) akan dinyalakan secara otomatis untuk menyambung ke SSID yang dikonfigurasi.

### 📊 2. Pembacaan Sensor (`bacaDHT`)
* Dibaca secara berkala setiap 2 detik sesuai spesifikasi hardware DHT22.
* Jika pembacaan menghasilkan `NaN` (*Not a Number*), variabel penghitung `gagalBaca` akan bertambah. 
* Jika kegagalan mencapai batas maksimal (`MAX_GAGAL = 5`), status layar akan berubah menjadi `Suhu:ERR`.

### 📺 3. Antarmuka Visual (`tampilkanOLED`)
* Baris 1: Menampilkan jenis koneksi yang aktif (`Ethernet`, `WiFi`, atau `Tidak ada`).
* Baris 2: Menampilkan IP lokal yang didapatkan dari server DHCP.
* Baris 3 & 4: Menampilkan nilai Suhu (°C) dan Kelembaban (%) dengan ukuran font yang lebih besar (Size 2) agar mudah dibaca.

### 🛡️ 4. Pengaman Sistem (`mulaiWatchdog` & `loop`)
* Fungsi `esp_task_wdt_reset()` dipanggil pada setiap awal siklus `loop()`.
* Jika terjadi kendala pada jaringan yang membuat sistem *stuck* lebih dari 30 detik, chip ESP32 akan melakukan *hardware reset* otomatis untuk memulihkan diri.

---

## 📦 Library yang Dibutuhkan
Pastikan library berikut sudah terinstal pada Arduino IDE Anda sebelum melakukan *compile*:
1. **DHT sensor library** (by Adafruit)
2. **Adafruit SSD1306** (by Adafruit)
3. **Adafruit GFX Library** (by Adafruit)
4. **Ethernet** (Library standard Arduino untuk modul W5500/ENC28J60)
