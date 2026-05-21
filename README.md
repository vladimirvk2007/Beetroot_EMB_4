# Beetroot_EMB_4 - PID Regulator Schematic (KiCad)

Цей репозиторій містить проєкт електричної схеми PID-регулятора, створений у KiCad 10.

## Огляд проєкту

- Назва проєкту: pid_regulator
- Файл проєкту: pid_regulator.kicad_pro
- Схема: pid_regulator.kicad_sch
- Версія формату KiCad у файлах: 10.0

## Структура репозиторію

- pid_regulator.kicad_pro - файл проєкту KiCad
- pid_regulator.kicad_sch - електрична схема
- pid_regulator.kicad_prl - локальні параметри перегляду/роботи з проєктом
- pid_regulator-backups/ - резервні копії KiCad
- fp-lib-table - таблиця бібліотек футпринтів
- sym-lib-table - таблиця бібліотек символів
- YD-ESP32-S3-main/ - локальна бібліотека символу, футпринта і 3D-моделі модуля YD-ESP32-S3

## Швидкий старт

1. Встановіть KiCad 10.x.
2. Відкрийте файл pid_regulator.kicad_pro.

## Експорт даних зі схеми

1. Відкрийте pid_regulator.kicad_sch.
2. Згенеруйте BOM (Tools -> Generate Bill of Materials), якщо потрібно.
3. Експортуйте netlist або інші звітні файли відповідно до вашого workflow.

## Нотатки

- Каталог YD-ESP32-S3-main містить сторонні KiCad-асети для плати YD-ESP32-S3 (символ, футпринт, STEP-модель).
- Рекомендовано не комітити тимчасові та великі згенеровані артефакти без потреби.
- Поточний репозиторій використовується як schematic-only проєкт (без PCB-етапу).
