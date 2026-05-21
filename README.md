# Beetroot_EMB_4 - PID Regulator (Both Directions)

Цей репозиторій містить KiCad-проєкт електричної схеми PID-регулятора у варіанті Both Directions.

## Огляд проєкту

- Назва проєкту: pid_regulator
- Назва схеми в title block: PID Regulator (Both Directions)
- Основний файл проєкту: pid_regulator.kicad_pro
- Основний файл схеми: pid_regulator.kicad_sch
- Версія KiCad (generator_version): 10.0

## Структура репозиторію

- pid_regulator.kicad_pro - файл проєкту KiCad
- pid_regulator.kicad_sch - електрична схема
- pid_regulator.kicad_prl - локальні налаштування перегляду
- pid_regulator.kicad_pcb - службовий файл плати (у цьому workflow основний фокус на схемі)
- pid_regulator-backups/ - резервні копії KiCad
- fp-lib-table - таблиця бібліотек футпринтів
- sym-lib-table - таблиця бібліотек символів
- YD-ESP32-S3-main/ - додаткові KiCad-ресурси для модуля YD-ESP32-S3

## Робота зі схемою

1. Встановіть KiCad 10.x.
2. Відкрийте pid_regulator.kicad_pro.
3. Перейдіть у Schematic Editor і відкрийте pid_regulator.kicad_sch.
4. Запустіть ERC та виправте знайдені попередження/помилки.
5. Перевірте номінали, анотації і зв'язність перед формуванням BOM.

## Експорт і звіти

1. У Schematic Editor відкрийте Tools -> Generate Bill of Materials для формування BOM.
2. За потреби експортуйте netlist або інші звіти відповідно до вашого процесу.

## Нотатки

- Поточна гілка використовується як схема для варіанту Both Directions.
- Якщо додаєте скрипти або правила перевірок, описуйте їх використання в цьому README.
