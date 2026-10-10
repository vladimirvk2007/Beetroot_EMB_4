# Beetroot EMB 4

Навчальний проєкт для STM32F411CE (плата BlackPill): I2C-модуль, приклад роботи з OLED SSD1306 та драйвер годинника DS1307.

## Що демонструє проєкт

Прошивка в `main_cpp()`:

1. Ініціалізує I2C1 (100 кГц).
2. Сканує шину та виводить адреси всіх знайдених пристроїв.
3. Ініціалізує SSD1306 (`OLED_Init`) за адресою `0x3C` у режимі 128x64.
4. Малює текст, прямокутник, коло та лінію, а раз на секунду оновлює лічильник.

Модулі `i2c` та `oled` використовують STM32 HAL. `oled` передає команди і framebuffer через наявний I2C API. ESP-IDF-компонент `k0i05/esp_ssd1306` напряму не підходить до цього проєкту, тому драйвер повторює його можливості та шрифти (MIT) під HAL; окрему залежність додавати не потрібно.

## Апаратна конфігурація

- I2C: I2C1, master, PB6 (SCL), PB7 (SDA), 100 кГц у прикладі, до 400 кГц
- MCU: STM32F411CEUx
- Framework: STM32Cube HAL
- Збірка та завантаження: PlatformIO, ST-Link

### Підключення OLED SSD1306

Під'єднайте SDA до PB7, SCL до PB6, а також живлення та GND. Драйвер підтримує панелі 128x32, 128x64 та 128x128; приклад налаштований на 128x64 і адресу `0x3C`. Якщо модуль має адресу `0x3D`, вкажіть її в `OLED_SettingsTypeDef.address`.

Усередині STM32 увімкнені слабкі підтягувальні резистори, але для надійної роботи на шині потрібні зовнішні (зазвичай вони вже є на модулі OLED).

## Збірка

```
pio run
pio run -t upload
```

## API модулів

### I2C (`Inc/i2c/i2c.h`)

Усі функції приймають 7-бітну адресу пристрою (без зсуву) і працюють у блокувальному режимі.

| Функція | Призначення |
|---------|-------------|
| `I2C_Init(&hi2c, I2C1, speed_hz)` | Ініціалізація I2C1 (master, 7-бітна адресація, до 400 кГц) |
| `I2C_Probe(&hi2c, addr, timeout)` | Перевірка, чи відповідає пристрій за адресою |
| `I2C_Scan(&hi2c, found, max_found, &count, timeout)` | Пошук усіх пристроїв в діапазоні `0x08`–`0x77`; `count` — загальна кількість знайдених |
| `I2C_ReadRegister(&hi2c, addr, reg, data, size, timeout)` | Читання регістрів з 8-бітною адресою регістра |
| `I2C_Transmit` / `I2C_Receive` | Пряма передача й прийом даних |

### OLED (`Inc/oled/oled.h`)

Драйвер SSD1306 (128x32, 128x64, 128x128) із прихованим дескриптором `OLED_HandleTypeDef *`. Усі функції повертають `HAL_StatusTypeDef`; зображення зберігається у framebuffer, а змінені сторінки одразу передаються на дисплей. Пам'ять під дескриптор і framebuffer береться з купи (`calloc`).

| Функція | Призначення |
|---------|-------------|
| `OLED_Init(&hi2c, &settings, &oled)` | Ініціалізація дисплея на шині I2C; `OLED_SETTINGS_128X64_DEFAULT()` — типові параметри |
| `OLED_Deinit(oled)` | Видалення дисплея, звільнення пам'яті |
| `OLED_Clear(oled)` | Очищення екрана |
| `OLED_DrawPixel` / `OLED_DrawLine` | Піксель, лінія |
| `OLED_DrawCircle` / `OLED_FillCircle` | Коло, залите коло |
| `OLED_DrawRectangle` / `OLED_FillRectangle` | Прямокутник, залитий прямокутник |
| `OLED_SetContrast(oled, 0..255)` | Яскравість |
| `OLED_Power(oled, on)` | Увімкнення / сплячий режим |
| `OLED_Print(oled, row, scale, invert, text)` | Текст шрифтом 8x8 у рядку сторінки (`scale` 1–3: до 16, 8, 5 символів) |
| `OLED_Printf(oled, row, scale, fmt, ...)` | Те саме з форматуванням `printf` (до 32 символів) |
| `OLED_PrintBdf(oled, font, x, y, text)` | Текст BDF-шрифтом (напр. `OLED_BdfFontNenr12_21x26`) у довільній позиції |

Параметри `OLED_SettingsTypeDef`: `address` (0 = `0x3C`), `scl_speed_hz` (0 = залишити поточну швидкість шини; інакше шина переналаштовується, до 400 кГц), `panel`, `flip` (поворот на 180°). У графічних функціях `invert = true` гасить пікселі.

Приклад:

```c
static I2C_HandleTypeDef hi2c1;

I2C_Init(&hi2c1, I2C1, 100000U);

OLED_SettingsTypeDef settings = OLED_SETTINGS_128X64_DEFAULT();
OLED_HandleTypeDef *oled = NULL;

OLED_Init(&hi2c1, &settings, &oled);
OLED_Print(oled, 0, 2, false, "Hello");
OLED_Printf(oled, 4, 1, "T=%d C", 25);
OLED_PrintBdf(oled, OLED_BdfFontNenr12_21x26, 0, 30, "12:34");
`````

### DS1307 (`Inc/ds1307/ds1307.h`)

Драйвер годинника реального часу DS1307 (I2C-адреса `0x68`) із прихованим дескриптором `DS1307_HandleTypeDef *`. Усі функції повертають `HAL_StatusTypeDef`; пам'ять під дескриптор береться з купи (`calloc`). Підтримується рік 2000–2099, `weekday`: 1 = неділя, ..., 7 = субота.

| Функція | Призначення |
| --- | --- |
| `DS1307_Init(&hi2c, &rtc)` | Додає годинник до шини I2C (перевіряє його наявність) |
| `DS1307_Deinit(rtc)` | Видаляє дескриптор; шина залишається відкритою |
| `DS1307_GetDateTime(rtc, &dt)` | Читає дату й час (приймає 12- і 24-годинний формат) |
| `DS1307_SetDateTime(rtc, &dt)` | Перевіряє й записує дату й час, запускає генератор |
| `DS1307_IsRunning(rtc, &running)` | Чи працює генератор (біт CH) |
| `DS1307_SetRunning(rtc, running)` | Зупиняє або запускає генератор |

Приклад:

```c
DS1307_HandleTypeDef *rtc = NULL;
DS1307_DateTimeTypeDef dt = {2026, 10, 11, 1, 12, 30, 0};

DS1307_Init(&hi2c1, &rtc);
DS1307_SetDateTime(rtc, &dt);
DS1307_GetDateTime(rtc, &dt);
```

## Структура

- `Src/app/main_app.cpp` — приклад: сканування I2C та малювання на OLED
- `Src/i2c/i2c.c`, `Inc/i2c/i2c.h` — I2C-модуль
- `Src/oled/oled.c`, `Inc/oled/oled.h` — драйвер SSD1306
- `Src/oled/oled_font_*.c`, `Inc/oled/oled_font.h` — шрифти 8x8 та BDF (MIT, з esp_ssd1306)
- `Src/ds1307/ds1307.c`, `Inc/ds1307/ds1307.h` — драйвер годинника DS1307
- `Src/main.c` — запуск HAL, системного тактування та периферії
- `Src/stm32f4xx_it.c` — обробники переривань
- `Src/stm32f4xx_hal_msp.c` — низькорівневе налаштування іншої периферії
- `Inc/stm32f4xx_hal_conf.h` — конфігурація HAL (увімкнено I2C)
