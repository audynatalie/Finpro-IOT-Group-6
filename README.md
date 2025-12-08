# ChickCare: An IoT-Based Intelligent Chick Incubator Monitoring and Control System 
[PPT Canva](https://www.canva.com/design/DAG61zygQrM/2T7orTyy49v0uax61qPRwg/view?utm_content=DAG61zygQrM&utm_campaign=designshare&utm_medium=link2&utm_source=uniquelinks&utlId=h6b4f7057a5)
[ChickCare Dashboard](https://chickcare.graycliff-7af4f541.eastus2.azurecontainerapps.io/dashboard/status)

## Introduction
Tingkat kematian anak ayam sangat tinggi pada dua minggu pertama setelah menetas akibat ketidakstabilan suhu dan kelembaban. Metode manual yang ada saat ini rentan terhadap human error, fluktuasi suhu ekstrem, dan kurangnya manajemen siklus cahaya yang penting untuk ritme sirkadian ayam.

Oleh karena itu, sistem ChickCare dikembangkan untuk menciptakan lingkungan inkubasi yang tepat dan bisa diakses dari jauh (remotely accessible), sehingga mengurangi keterlibatan manual dengan meningkatkan kelangsungan hidup anak ayam.

### Tujuan Proyek ChickCare antara lain:
- Menciptakan sistem yang secara otomatis menyesuaikan target suhu ideal sesuai tahapap pertumbuhan dalam 2 minggu pertama dan mempertahankannya menggunakan PWM pada Lampu Brooder untuk keakuratan yang maksimal.
- Mengimplementasikan logika kontrol yang mengatur level kelembaban melalui Dehumidifier Spray dan menggabungkan penjadwalan terang dan gelap yang terorganisir.
- Menyediakan indikator visual (LED) dan monitor data (Serial Output) untuk melaporkan status sistem, nilai PWM dinamis, dan memberikan alert saat terjadi kondisi kritis.
Menginteintegrasikan dengan Node-RED dengan terhubung dari data sensor ke Gemini AI. Fitur ini menganalisis kondisi suhu dan kelembaban dan rekomendasi secara langsung kepada peternak.

## Implementation

Sistem ini dibangun menggunakan arsitektur *multitasking* berbasis **FreeRTOS** pada mikrokontroler ESP32 untuk memastikan pembacaan sensor dan kontrol aktuator berjalan tanpa hambatan (*non-blocking*).

### Hardware Design
Perangkat keras utama yang digunakan meliputi:
- **Microcontroller:** ESP32 Dev Module.
- **Sensor:** DHT22 (untuk akurasi suhu dan kelembaban).
- **Actuators:**
  - *Brooder Lamp* (dikontrol via Relay & PWM Logic).
  - *Dehumidifier Spray* (dikontrol via Relay).
- **Visual Indicators:** LED indikator untuk status Heater dan Spray.

### Software Architecture
- **FreeRTOS Tasks:** Sistem dibagi menjadi *tasks* terpisah (Temperature Task, Control Task) yang berjalan secara paralel.
- **PWM Control Logic:** Mengatur intensitas panas lampu secara dinamis (0-255) berdasarkan selisih suhu aktual dan target.
- **Node-RED & AI:** Bertindak sebagai *dashboard* visualisasi dan jembatan komunikasi data ke Gemini AI untuk analisis cerdas.

## Testing and Evaluation

Pengujian dilakukan menggunakan dua metode yaitu simulasi digital melalui **Wokwi** dan implementasi pada **Rangkaian Fisik**. Fokus pengujian adalah validasi respons sistem terhadap perubahan suhu ekstrem dan kelembaban.

### 1. Temperature Control Response (PWM)
Sistem berhasil mempertahankan suhu target ($32.0^{\circ}C - 33.0^{\circ}C$) dengan menyesuaikan daya pemanas secara otomatis. Berikut adalah hasil respons logika PWM terhadap kondisi lingkungan:

| Kondisi Lingkungan | Status Suhu | Output PWM | Respons Aktuator |
| :--- | :--- | :--- | :--- |
| **Suhu Dingin** (< 32°C) | *Undershoot* | **255 (Max)** | Lampu menyala terang maksimal untuk pemanasan cepat. |
| **Suhu Optimal** (32-33°C) | *Stable* | **128 (Med)** | Lampu meredup (setengah daya) untuk menjaga suhu stabil. |
| **Suhu Panas** (> 33.5°C) | *Overshoot* | **0 (Off)** | Lampu mati total untuk mencegah *overheating*. |

### 2. Humidity Control (Hysteresis & Non-Blocking)
Pengujian fungsi *Dehumidifier Spray* menunjukkan bahwa sistem mampu menjaga kelembaban tanpa mengganggu pembacaan sensor suhu (logika *non-blocking* menggunakan `millis()`).

- **Threshold ON:** Spray aktif saat kelembaban turun di bawah **60%**.
- **Threshold OFF:** Spray mati otomatis saat kelembaban naik di atas **62%**.
- **Durasi:** Spray menyala dalam *burst* singkat (3 detik) untuk menghindari kelembaban berlebih.

### 3. Evaluation Result
Berdasarkan pengujian fisik dan simulasi:
- Logika PWM terbukti lebih efisien dibandingkan kontrol *On/Off* biasa karena mampu mengurangi fluktuasi suhu yang drastis.
- Penggunaan FreeRTOS berhasil mencegah *data race* dan memastikan sensor tetap responsif meskipun aktuator (spray) sedang aktif.
- Sistem secara konsisten kembali ke titik setimbang (*setpoint*) setelah diberikan gangguan eksternal (suhu dingin/panas buatan).

### Simulation Link
- Wokwi: https://wokwi.com/projects/449672325379650561

## Conclusion
ChickCare berhasil mengimplementasikan sistem inkubator cerdas yang mampu beradaptasi dengan kondisi lingkungan secara mandiri. Dengan menggabungkan kontrol PWM presisi dan manajemen *task* FreeRTOS, sistem ini menawarkan solusi efektif untuk menurunkan angka kematian anak ayam pada fase *brooding*.

## References
- [1] Nutrena, “Heat Lamps For Chicks,” Nutrena Animal Feeds, Jan. 18, 2024. https://nutrenaworld.com/heat-lamps-for-chicks/ (accessed Dec. 02, 2025). 
- [2] Czarick, Michael, dkk. “Poultry Housing Tips Do Chicks Benefit From 24 Hours of Light?” 2022. Department of Poultry Science of University of Georgia [Online]. Available: https://www.poultryventilation.com/wp-content/uploads/vol34n7.pdf (accessed Dec. 03, 2025) 
