# ChickCare: An IoT-Based Intelligent Chick Incubator Monitoring and Control System 

## Introduction
Tingkat kematian anak ayam sangat tinggi pada dua minggu pertama setelah menetas akibat ketidakstabilan suhu dan kelembaban. Metode manual yang ada saat ini rentan terhadap human error, fluktuasi suhu ekstrem, dan kurangnya manajemen siklus cahaya (siang/malam) yang penting untuk ritme sirkadian ayam.

Oleh karena itu, sistem ChickCare dikembangkan untuk menciptakan lingkungan inkubasi yang tepat dan bisa diakses dari jauh (remotely accessible), sehingga mengurangi keterlibatan manual dengan meningkatkan kelangsungan hidup anak ayam.

### Tujuan Proyek ChickCare antara lain:
- Menciptakan sistem yang secara otomatis menyesuaikan target suhu ideal sesuai tahapap pertumbuhan dalam 2 minggu pertama dan mempertahankannya menggunakan PWM pada Lampu Brooder untuk keakuratan yang maksimal.
- Mengimplementasikan logika kontrol yang mengatur level kelembaban melalui Dehumidifier Spray dan menggabungkan penjadwalan terang dan gelap yang terorganisir (Jadwal Cahaya) sesuai mode DAY/NIGHT.
- Menyediakan indikator visual (LED) dan monitor data (Serial Output) untuk melaporkan status sistem, nilai PWM dinamis, dan memberikan alert saat terjadi kondisi kritis.
Menginteintegrasikan dengan Node-RED dengan terhubung dari data sensor ke Gemini AI. Fitur ini menganalisis kondisi suhu dan kelembaban dan rekomendasi secara langsung kepada peternak.

## Implementation
## Testing and Evaluation

- Wokwi: https://wokwi.com/projects/449672325379650561
## Conclusion
## References
- [1] Nutrena, “Heat Lamps For Chicks,” Nutrena Animal Feeds, Jan. 18, 2024. https://nutrenaworld.com/heat-lamps-for-chicks/ (accessed Dec. 02, 2025). 
- [2] Czarick, Michael, dkk. “Poultry Housing Tips Do Chicks Benefit From 24 Hours of Light?” 2022. Department of Poultry Science of University of Georgia [Online]. Available: https://www.poultryventilation.com/wp-content/uploads/vol34n7.pdf (accessed Dec. 03, 2025) 
