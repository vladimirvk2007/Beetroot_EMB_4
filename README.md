# ESP32-S3 ADC Attenuation Demo

PlatformIO-проєкт для ESP32-S3, який демонструє роботу АЦП з подільником 10k/1k та порівнює:

- розрахункову напругу на вході АЦП (Vadc_calc)
- скомпенсовану напругу з калібруванням (Vadc_comp)

## Що саме тестується

Для GPIO4 по черзі встановлюються атенюатори:

- 0 dB
- 2.5 dB
- 6 dB
- 11 dB

Для кожного режиму виконується усереднення вимірювань та друк у Serial:

Atten | RAW | Vadc_calc | Vadc_comp

## Формули

- Vadc_calc = RAW / ADC_MAX_CODE * Vfs
- Vadc_comp = analogReadMilliVolts(ADC_PIN) / 1000
- Divider ratio = (Rtop + Rbot) / Rbot = (10000 + 1000) / 1000 = 11

Де Vfs задається окремо для кожного атенюатора в таблиці конфігурації.

## Апаратна частина

- Плата: YD-ESP32-S3 (ESP32-S3-N16R8)
- Вхід АЦП: GPIO4
- Подільник: 10k (верхній резистор) / 1k (нижній резистор)

## Налаштування в коді

Основні параметри в src/main.cpp:

- ADC_PIN
- ADC_MAX_CODE
- DIVIDER_TOP_R_OHM
- DIVIDER_BOT_R_OHM
- SAMPLES_PER_POINT
- ATTENUATION_SETTLE_DELAY_MS
- LOOP_DELAY_MS

## Збірка і запуск

```bash
pio run
pio run -t upload
pio run -t monitor
```

Або однією командою:

```bash
pio run -t upload -t monitor
```
