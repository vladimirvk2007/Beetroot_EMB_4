# Beetroot EMB 4 (STM32F411 + FreeRTOS + USB CDC)

Демонстраційний проєкт для плати BlackPill (STM32F411CE), згенерований на базі STM32CubeMX і зібраний через PlatformIO.

Проєкт показує:
- запуск FreeRTOS (CMSIS-RTOS v2 wrapper);
- керування кількома задачами (створення, suspend/resume, notify, delete);
- діагностику задач (стан, пріоритет, stack high water mark);
- вивід логів через USB CDC (virtual COM port);
- керування LED через невелику C++-абстракцію.

## Апаратна платформа

- MCU: STM32F411CEU6
- Board: WeAct BlackPill V2.0 (env: blackpill_f411ce)
- Framework: stm32cube
- Завантаження прошивки: ST-Link

## Структура проєкту

- src/Core/Src/main.c: старт системи, ініціалізація HAL/clock/RTOS, виклик main_cpp() з defaultTask.
- src/Core/Src/main_app.cpp: прикладна логіка FreeRTOS (LED задачі, heartbeat, controller, USB логування).
- src/Core/Inc/led.h: C++ клас Led для керування GPIO.
- src/Core/Inc/FreeRTOSConfig.h: конфігурація FreeRTOS (увімкнені API, пріоритети, heap).
- src/USB_DEVICE/App/usbd_cdc_if.c: USB CDC інтерфейс.
- platformio.ini: налаштування середовища build/flash для PlatformIO.

## Що робить застосунок

Після старту scheduler створюються задачі:

- led_fast_task: блимання LED_FAST з періодом 250 мс.
- led_medium_task: блимання LED_MEDIUM з періодом 700 мс.
- led_slow_task: блимання LED_SLOW з періодом 1300 мс.
- heartbeat_task: друк [HEARTBEAT] раз на 1 секунду.
- controller_task: кожні 5 секунд демонструє API FreeRTOS на selected_handle (зараз led_medium_task):
	- eTaskGetState
	- uxTaskPriorityGet
	- vTaskSuspend / vTaskResume
	- xTaskNotifyGive
	- xTaskAbortDelay
	- uxTaskGetStackHighWaterMark
	- vTaskDelete

Логи виводяться через printf, який перенаправлено у USB CDC (функція _write у main_app.cpp).

## LED мапінг

У поточній конфігурації:

- LED_FAST: PC13
- LED_MEDIUM: PA0
- LED_SLOW: PA1

Увага: в led.h задано активний низький рівень:

- LED_ON_LEVEL = GPIO_PIN_RESET
- LED_OFF_LEVEL = GPIO_PIN_SET

Для більшості BlackPill це коректно для вбудованого LED на PC13.

## Вимоги

- VS Code + PlatformIO extension
- ST-Link (для прошивки)
- USB кабель для живлення/CDC

## Збірка

З кореня репозиторію:

pio run

## Прошивка

pio run -t upload

Якщо ST-Link не бачиться, перевірте підключення SWDIO/SWCLK/GND/3V3 та драйвери ST-Link.

## Серійний монітор (USB CDC)

Після старту USB пристрою зʼявляється virtual COM port.

Команда:

pio device monitor -b 115200

Примітка: для CDC реальна швидкість не критична, але 115200 зручно як дефолт у monitor.

## Налаштування FreeRTOS, важливе для демо

Щоб виклик xTaskAbortDelay лінкувався, у FreeRTOSConfig.h має бути:

- INCLUDE_xTaskAbortDelay 1

У цьому проєкті параметр уже увімкнено.

## Типовий цикл перевірки

1. Зібрати: pio run
2. Прошити: pio run -t upload
3. Відкрити monitor: pio device monitor -b 115200
4. Перевірити в логах:
	 - [HEARTBEAT] раз на секунду
	 - перемикання LED задач
	 - кроки controller_task з викликами API

## Troubleshooting

- Undefined reference to xTaskAbortDelay:
	- Додайте/перевірте INCLUDE_xTaskAbortDelay 1 у src/Core/Inc/FreeRTOSConfig.h
	- Перезберіть проєкт: pio run -t clean && pio run

- Немає логів у monitor:
	- Переконайтесь, що USB CDC ініціалізовано (MX_USB_DEVICE_Init викликається в StartDefaultTask).
	- Перевірте, що обрано правильний COM порт.

- Build проходить, але LED не блимає:
	- Перевірте мапінг GPIO в main_app.cpp і фактичне підключення LED на вашій платі.

