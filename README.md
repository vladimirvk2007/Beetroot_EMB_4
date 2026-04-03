**ESP32 Servo Demo**

Демонстраційний проект для керування сервоприводом за допомогою ESP32 та бібліотеки ESP32Servo.

---

**Опис:**
	- Керує стандартним сервоприводом через PWM (GPIO 18).
	- Серво плавно рухається від 0° до 180° і назад.
	- Вивід поточної позиції у Serial Monitor (115200 бод).

**Файл з кодом:**
	- src/main.cpp

**Обладнання:**
	- Плата: ESP32-S3
	- Серво: стандартний 5В сервопривід
	- PWM сигнал: GPIO 18 (можна змінити у main.cpp)

**Залежності:**
	- [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) (додається автоматично через PlatformIO)

**Збірка та прошивка (PlatformIO):**
```bash
pio run
pio run -t upload
pio run -t monitor
```
