
# ESP32-S3 Encoder Example

Цей проект демонструє роботу з енкодером на ESP32-S3 (ESP-IDF, PlatformIO):
- Зчитування позиції енкодера (PCNT, quadrature X4)
- Обробка кнопки енкодера
- Керування світлодіодом (LED) залежно від стану кнопки
- Логування стану енкодера та GPIO

## Основні файли
- Код: `src/main.cpp`
- Налаштування: `platformio.ini`

## Основні налаштування
- **ENCODER_A_INPUT**: GPIO 17
- **ENCODER_B_INPUT**: GPIO 16
- **ENCODER_BUTTON_INPUT**: GPIO 15 (кнопка, активний low)
- **LED_OUTPUT**: GPIO 4
- **Дебаунс**: 1000 ns (через PCNT glitch filter)

## Збірка та прошивка
1. Підключіть ESP32-S3 до комп'ютера
2. Відкрийте термінал у корені проекту
3. Зберіть та прошийте:
   ```
   pio run -t upload
   ```
4. Для перегляду логів:
   ```
   pio device monitor
   ```

## Логування
Вивід у консоль (кожні 500 мс):
```
I (1234) Encoder: Position: 42 | A: 1 | B: 0
```
де:
- Position — поточне значення лічильника енкодера
- A, B — стани відповідних GPIO

Світлодіод вмикається при натисканні кнопки (активний low).

## Додатково
- Для зміни debounce змініть ENCODER_DEBOUNCE_NS у main.cpp
- Для зміни GPIO — відповідні макроси у main.cpp

Проект використовує ESP-IDF 5.x та PlatformIO.

