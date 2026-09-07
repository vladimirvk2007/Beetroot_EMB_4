# Beetroot EMB 4

Базовий STM32Cube + PlatformIO шаблон для BlackPill STM32F411CE.

## Поточний стан

- MCU: STM32F411CEUx
- Framework: `stm32cube`
- Build: PlatformIO
- Upload: ST-Link
- USB: OTG FS, CDC Virtual COM Port
- LED: PC13, active-high

## Структура

- `Src/app` - прикладна логіка
- `Src/printf` - функція `printf()` на USB
- `Inc/printf` - заголовки для виводу

## Поведінка прошивки

- `main.c` виконує HAL/Clock/GPIO/USB ініціалізацію
- `main_cpp()` викликається з C-коду
- LED на PC13 перемикається кожні 0,5 секунди
- `printf()` виводиться у USB

## Примітки

- Основний C++ файл: `Src/app/main_app.cpp`
- `printf()` ретаргетиться у `Src/printf/usb_printf.c`
- USB Type-C у поточній конфігурації використовується для логування, не для прошивки
