#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Inisialisasi LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Inisialisasi Servo
Servo katupAir;

// Definisi pin
const int pinSoilMoisture = A0;
const int pinRaindrop = A1;
const int pinLampuMerah = 2;
const int pinLampuKuning = 3;
const int pinLampuHijau = 4;
const int pinLampuPutih = 5;
const int pinMicroServo = 7;

// Variabel untuk fuzzyfikasi
int kelembapanTanah;
int curahHujan;
float persentaseCurahHujan;
int z;

// Fungsi fuzzyfikasi
float fuzzyKelembapanTanah(int x, int a, int b) {
  if (x <= a) {
    return 1.0; // Basah
  } else if (x >= b) {
    return 0.0; // Kering
  } else {
    return (float)(b - x) / (b - a); // Lembab
  }
}

float fuzzyCurahHujan(int y, int a, int b) {
  if (y <= a) {
    return 0.0; // Rendah
  } else if (y >= b) {
    return 1.0; // Tinggi
  } else {
    return (float)(y - a) / (b - a); // Sedang
  }
}

float fuzzyKatupAir(int z, int a, int b) {
  if (z <= a) {
    return 1.0; // Sedikit
  } else if (z >= b) {
    return 0.0; // Banyak
  } else {
    return (float)(b - z) / (b - a); // Sedang
  }
}

void setup() {
  // Inisialisasi pin
  pinMode(pinSoilMoisture, INPUT);
  pinMode(pinRaindrop, INPUT);
  pinMode(pinLampuMerah, OUTPUT);
  pinMode(pinLampuKuning, OUTPUT);
  pinMode(pinLampuHijau, OUTPUT);
  pinMode(pinLampuPutih, OUTPUT);
  katupAir.attach(pinMicroServo);

  // Inisialisasi LCD
  lcd.begin(16, 2);
  lcd.print("Fuzzy Mamdani");
}

void loop() {
  // Baca nilai sensor
  kelembapanTanah = analogRead(pinSoilMoisture);
  curahHujan = analogRead(pinRaindrop);

  // Konversi nilai curah hujan ke dalam persentase
  persentaseCurahHujan = map(curahHujan, 0, 1023, 0, 100);

  // Fuzzyfikasi
  float muBasah = fuzzyKelembapanTanah(kelembapanTanah, 0, 300);
  float muLembab = fuzzyKelembapanTanah(kelembapanTanah, 301, 899);
  float muKering = fuzzyKelembapanTanah(kelembapanTanah, 900, 1023);

  float muRendah = fuzzyCurahHujan(persentaseCurahHujan, 0, 30);
  float muSedang = fuzzyCurahHujan(persentaseCurahHujan, 31, 70);
  float muTinggi = fuzzyCurahHujan(persentaseCurahHujan, 71, 100);

  // Inferensi
  float alphaPred1 = min(muKering, muRendah);
  float alphaPred2 = min(muKering, muSedang);
  float alphaPred3 = min(muKering, muTinggi);
  float alphaPred4 = min(muLembab, muRendah);
  float alphaPred5 = min(muLembab, muSedang);
  float alphaPred6 = min(muLembab, muTinggi);
  float alphaPred7 = min(muBasah, muRendah);
  float alphaPred8 = min(muBasah, muSedang);
  float alphaPred9 = min(muBasah, muTinggi);

  // Defuzzifikasi
  z = (alphaPred1 * 5 + alphaPred2 * 10 + alphaPred3 * 20 + alphaPred4 * 5 +
       alphaPred5 * 10 + alphaPred6 * 20 + alphaPred7 * 0 + alphaPred8 * 0 +
       alphaPred9 * 0) /
      (alphaPred1 + alphaPred2 + alphaPred3 + alphaPred4 + alphaPred5 +
       alphaPred6 + alphaPred7 + alphaPred8 + alphaPred9);

  // Kendali servo dan lampu
  if (z > 0) {
    katupAir.write(90); // Buka katup air
  } else {
    katupAir.write(0); // Tutup katup air
  }

  // Kendali lampu
  digitalWrite(pinLampuMerah, muKering);
  digitalWrite(pinLampuKuning, muLembab);
  digitalWrite(pinLampuHijau, muBasah);
  digitalWrite(pinLampuPutih, persentaseCurahHujan > 0);

  // Tampilkan informasi di LCD dan serial monitor
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("hujan=");
  lcd.setCursor(6, 0);
  lcd.print(persentaseCurahHujan);
  lcd.setCursor(12, 0);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("tanah=");
  lcd.setCursor(6, 1);
  lcd.print(kelembapanTanah);

  Serial.print("Air Banyak: ");
  Serial.print(muBasah);
  Serial.print(", Air Sedang: ");
  Serial.print(muLembab);
  Serial.print(", Air Sedikit: ");
  Serial.print(muKering);
  Serial.print(", Tidak Disiram: ");
  Serial.print(persentaseCurahHujan == 0);
  Serial.print(", Z: ");
  Serial.println(z);

  delay(1000); // Delay untuk stabilisasi nilai
}