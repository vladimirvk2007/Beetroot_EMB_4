# Beetroot EMB 4

Базовий STM32Cube + PlatformIO шаблон для BlackPill STM32F411CE.

## Поточний стан

- MCU: STM32F411CEUx
- Framework: `stm32cube`
- Build: PlatformIO
- Upload: ST-Link
- USB: OTG FS, CDC Virtual COM Port
- LED: PC13, active-low

## Структура

- `Src/app` - прикладна логіка на C++
- `Src/printf` - retarget `printf()` на USB CDC
- `Inc/led` - модуль LED
- `Inc/printf` - заголовки для виводу

## Поведінка прошивки

- `main.c` виконує HAL/Clock/GPIO/USB ініціалізацію
- `main_cpp()` викликається з C-коду
- LED на PC13 перемикається кожну 1 секунду
- `printf()` виводиться у USB CDC

## Примітки

- Основний C++ файл: `Src/app/main_app.cpp`
- LED модуль: `Led(GPIOC, GPIO_PIN_13, false)`
- `printf()` ретаргетиться у `Src/printf/usb_printf.c`
- USB Type-C у поточній конфігурації використовується для CDC, не для прошивки
