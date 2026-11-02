# AlashSettings: руководство / нұсқаулық / guide

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

**Настройки, которые переживают выключение**, одной структурой: `settings.data.volume = 7; settings.save();`. Контрольная сумма, версия, значения по умолчанию, **ротация ячеек** (EEPROM служит в разы дольше), **защита от обрыва питания во время записи**, перенос данных со старой версии и автосохранение. Работает на Arduino, ESP8266 и ESP32. Без зависимостей.

### Установка
Arduino IDE: Скетч, Подключить библиотеку, Управлять библиотеками, найти **AlashSettings** и нажать Установить. Или скачайте ZIP с GitHub: Скетч, Подключить библиотеку, Добавить .ZIP библиотеку.

### Платы
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 и другие, где работает Arduino-ядро.

### Быстрый старт
```cpp
#include <AlashSettings.h>
struct Config { uint8_t volume; uint16_t boots; };
AlashSettings<Config> settings;          // версия 1, адрес 0, 4 ячейки

void setup() {
  Config defaults = {5, 0};
  settings.begin(defaults);              // из памяти или значения по умолчанию
  settings.data.boots++;
  settings.save();                       // пишет только если что-то изменилось
}
void loop() {}
```

### Что умеет
- **Одна структура:** `settings.data.поле`, читайте и меняйте как обычно; `save()` пишет, только если есть изменения.
- **Значения по умолчанию:** при первом запуске, смене версии или порче данных берутся ваши defaults; `loadResult()` скажет, что случилось.
- **Контрольная сумма CRC16 и проверка чтением:** мусор из EEPROM не примется за настройки.
- **Ротация ячеек:** запись идёт по кругу в 1..16 ячеек, износ делится на их число.
- **Обрыв питания безопасен:** недописанная ячейка не пройдёт проверку, прежние данные остаются целыми (проверено обрывом на каждом байте).
- **Миграция:** `onMigrate()` переносит данные старой версии, даже если размер структуры изменился.
- **Автосохранение:** `autosave(мс)` и `update()` пишут только после того, как значение перестало меняться.
- **Несколько наборов:** разные адреса в EEPROM не мешают друг другу.
- **Видно, что происходит:** `diagnose(Serial)` объясняет результат загрузки и ресурс EEPROM на трёх языках.
- Работает на **Arduino AVR, ESP8266, ESP32** (`EEPROM.begin()` и `commit()` выполняются сами).

### Справочник по методам
| Метод | Что делает |
|---|---|
| `AlashSettings<T>(version, address, slots)` | Хранилище структуры T: версия, адрес в EEPROM, число ячеек (1..16) |
| `begin(defaults)` | Загрузить или взять defaults; true, если из памяти |
| `data` | Ваша структура: читайте и меняйте поля |
| `save() / saveNow() / changed()` | Записать при изменениях / принудительно / есть ли изменения |
| `autosave(ms) + update()` | Запись после тишины ms; update() в loop() |
| `onMigrate(fn)` | Функция переноса данных старой версии |
| `reset(defaults) / erase()` | Вернуть defaults и сохранить / стереть всё |
| `loadResult() / wasReset()` | Что было при begin(): LOADED, FIRST_BOOT, VERSION_CHANGED, CORRUPT, MIGRATED |
| `validSlots() / sequence() / saves()` | Исправные ячейки, счётчик записей, записей с запуска |
| `diagnose(Serial) / errorText() / setLanguage()` | Самопроверка и сообщения на ru/kk/en |

### Если что-то не работает
**После обновления прошивки настройки сбросились**

Вы изменили структуру. Поднимите версию (`AlashSettings<Config> s(2)`) и задайте `onMigrate()`, если нужны старые значения (пример Migration). `loadResult()` покажет VERSION_CHANGED.

**Ошибка ERR_TOO_BIG**

Структура умножается на число ячеек. Уменьшите структуру или третий параметр (`AlashSettings<Config> s(1, 0, 2)`). У Uno всего 1024 байта EEPROM.

**Настройки портятся или сбрасываются сами**

Другой код (или другой AlashSettings) пишет в те же ячейки EEPROM. Разнесите адреса: второй параметр конструктора. Диагностика покажет CORRUPT.

**EEPROM быстро изнашивается**

Не вызывайте `save()` в каждом проходе `loop()`. Используйте `autosave(3000)` и `update()` или сохраняйте по событию. Для частых данных (счётчики) лучше RTC-память или SD.

**На ESP32 пишет, но после перезагрузки пусто**

Библиотека сама вызывает `EEPROM.begin()` и `commit()`. Не вызывайте свой `EEPROM.begin()` с меньшим размером и не используйте те же адреса. `errorText()` покажет ERR_WRITE_FAILED, если commit не удался.

### Самопроверка
В библиотеке есть `diagnose(Serial)`: она печатает, что видит плата, и подсказывает, что проверить. Язык подсказок: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Примеры
`Basic`, `Autosave`, `Migration`, `Diagnose`

---

## Қазақша

**Өшіргенде сақталатын баптаулар** бір құрылыммен: `settings.data.volume = 7; settings.save();`. Бақылау сомасы, нұсқа, әдепкі мәндер, **ұяшықтар айналымы** (EEPROM есе ұзақ қызмет етеді), **жазу кезінде қуат үзілуінен қорғау**, ескі нұсқадан деректерді көшіру және автосақтау. Arduino, ESP8266 және ESP32 үшін. Тәуелділіксіз.

### Орнату
Arduino IDE: Sketch, Include Library, Manage Libraries, **AlashSettings** табып, Install басыңыз. Немесе GitHub-тан ZIP жүктеп, Sketch, Include Library, Add .ZIP Library.

### Тақталар
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 және Arduino ядросы бар басқалар.

### Жылдам бастау
```cpp
#include <AlashSettings.h>
struct Config { uint8_t volume; uint16_t boots; };
AlashSettings<Config> settings;          // версия 1, адрес 0, 4 ячейки

void setup() {
  Config defaults = {5, 0};
  settings.begin(defaults);              // из памяти или значения по умолчанию
  settings.data.boots++;
  settings.save();                       // пишет только если что-то изменилось
}
void loop() {}
```

### Мүмкіндіктері
- **Бір құрылым:** `settings.data.өріс`, әдеттегідей оқып өзгертіңіз; `save()` тек өзгеріс болса жазады.
- **Әдепкі мәндер:** алғашқы іске қосуда, нұсқа өзгергенде немесе деректер бүлінгенде defaults алынады; `loadResult()` не болғанын айтады.
- **CRC16 бақылау сомасы және оқып тексеру:** EEPROM қоқысы баптау болып қабылданбайды.
- **Ұяшықтар айналымы:** жазу 1..16 ұяшық бойынша айналады, тозу ұяшық санына бөлінеді.
- **Қуат үзілуі қауіпсіз:** аяқталмаған ұяшық тексеруден өтпейді, бұрынғы деректер бүтін қалады (әр байтта үзіп тексерілген).
- **Көшіру:** `onMigrate()` құрылым өлшемі өзгерсе де ескі нұсқа деректерін көшіреді.
- **Автосақтау:** `autosave(мс)` және `update()` мән өзгеруін тоқтатқаннан кейін ғана жазады.
- **Бірнеше жиын:** EEPROM-дағы әртүрлі мекенжайлар бір-біріне кедергі жасамайды.
- **Не болып жатқаны көрінеді:** `diagnose(Serial)` жүктеу нәтижесін және EEPROM ресурсын үш тілде түсіндіреді.
- **Arduino AVR, ESP8266, ESP32** (`EEPROM.begin()` және `commit()` өзі орындалады).

### Әдістер анықтамасы
| Әдіс | Не істейді |
|---|---|
| `AlashSettings<T>(version, address, slots)` | T құрылымының қоймасы |
| `begin(defaults)` | Жүктеу немесе defaults алу |
| `data` | Сіздің құрылымыңыз |
| `save() / saveNow() / changed()` | Өзгеріс болса жазу |
| `autosave(ms) + update()` | Тыныштықтан кейін жазу |
| `onMigrate(fn)` | Ескі нұсқаны көшіру |
| `reset(defaults) / erase()` | Defaults қайтару / өшіру |
| `loadResult() / wasReset()` | begin() кезінде не болды |
| `validSlots() / sequence() / saves()` | Ұяшықтар, есептегіштер |
| `diagnose(Serial) / errorText() / setLanguage()` | Өзін-өзі тексеру |

### Бірдеңе жұмыс істемесе
**Прошивкадан кейін баптаулар тасталды**

Құрылымды өзгерттіңіз. Нұсқаны көтеріңіз (`AlashSettings<Config> s(2)`) және ескі мәндер керек болса `onMigrate()` беріңіз (Migration мысалы).

**ERR_TOO_BIG қатесі**

Құрылым ұяшықтар санына көбейтіледі. Құрылымды немесе үшінші параметрді азайтыңыз. Uno-да барлығы 1024 байт EEPROM.

**Баптаулар бүлінеді немесе өздігінен тасталады**

Басқа код (немесе басқа AlashSettings) сол EEPROM ұяшықтарына жазады. Мекенжайларды бөліңіз: конструктордың екінші параметрі.

**EEPROM тез тозады**

`save()` әр `loop()` сайын шақырмаңыз. `autosave(3000)` және `update()` қолданыңыз немесе оқиға бойынша сақтаңыз.

**ESP32-де жазады, бірақ қайта жүктегеннен кейін бос**

Кітапхана `EEPROM.begin()` және `commit()` өзі шақырады. Кішірек өлшемді өз `EEPROM.begin()` шақырмаңыз. commit сәтсіз болса `errorText()` ERR_WRITE_FAILED көрсетеді.

### Өзін-өзі тексеру
Кітапханада `diagnose(Serial)` бар: тақта не көргенін басып шығарады және нені тексеру керегін айтады. Кеңес тілі: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Мысалдар
`Basic`, `Autosave`, `Migration`, `Diagnose`

---

## English

**Settings that survive power-off** in one struct: `settings.data.volume = 7; settings.save();`. Checksum, version, defaults, **slot rotation** (the EEPROM lasts many times longer), **protection from power loss during a write**, migration from an old version and autosave. Works on Arduino, ESP8266 and ESP32. No dependencies.

### Installation
Arduino IDE: Sketch, Include Library, Manage Libraries, search **AlashSettings** and click Install. Or download the ZIP from GitHub: Sketch, Include Library, Add .ZIP Library.

### Boards
Arduino Uno / Nano / Mega (AVR), ESP32, ESP8266 and others with an Arduino core.

### Quick start
```cpp
#include <AlashSettings.h>
struct Config { uint8_t volume; uint16_t boots; };
AlashSettings<Config> settings;          // версия 1, адрес 0, 4 ячейки

void setup() {
  Config defaults = {5, 0};
  settings.begin(defaults);              // из памяти или значения по умолчанию
  settings.data.boots++;
  settings.save();                       // пишет только если что-то изменилось
}
void loop() {}
```

### Features
- **One struct:** `settings.data.field`, read and change as usual; `save()` writes only if something changed.
- **Defaults:** on first start, a version change or corrupt data your defaults are used; `loadResult()` tells what happened.
- **CRC16 checksum and read-back verification:** garbage in the EEPROM is never taken for settings.
- **Slot rotation:** writes go round-robin over 1..16 slots, wear is divided by their number.
- **Power loss is safe:** a half-written slot fails the check, the previous data stays intact (tested by cutting power at every byte).
- **Migration:** `onMigrate()` carries old data over even if the struct size changed.
- **Autosave:** `autosave(ms)` and `update()` write only after the value stops changing.
- **Several sets:** different EEPROM addresses do not interfere.
- **See what happens:** `diagnose(Serial)` explains the load result and the EEPROM endurance in three languages.
- Works on **Arduino AVR, ESP8266, ESP32** (`EEPROM.begin()` and `commit()` are done for you).

### Method reference
| Method | What it does |
|---|---|
| `AlashSettings<T>(version, address, slots)` | Storage of struct T: version, EEPROM address, slot count (1..16) |
| `begin(defaults)` | Load or take defaults; true if loaded from memory |
| `data` | Your struct: read and change fields |
| `save() / saveNow() / changed()` | Write if changed / force / has changes |
| `autosave(ms) + update()` | Write after ms of quiet; update() in loop() |
| `onMigrate(fn)` | Function that migrates old-version data |
| `reset(defaults) / erase()` | Restore defaults and save / erase all |
| `loadResult() / wasReset()` | What happened at begin() |
| `validSlots() / sequence() / saves()` | Valid slots, write counter, writes since start |
| `diagnose(Serial) / errorText() / setLanguage()` | Self-check and messages in ru/kk/en |

### If something does not work
**Settings reset after a firmware update**

You changed the struct. Raise the version (`AlashSettings<Config> s(2)`) and set `onMigrate()` if you need the old values (Migration example). `loadResult()` shows VERSION_CHANGED.

**ERR_TOO_BIG error**

The struct is multiplied by the slot count. Shrink the struct or the third parameter (`AlashSettings<Config> s(1, 0, 2)`). Uno has only 1024 bytes of EEPROM.

**Settings get corrupted or reset by themselves**

Other code (or another AlashSettings) writes to the same EEPROM cells. Separate the addresses: the second constructor parameter. Diagnostics shows CORRUPT.

**The EEPROM wears out fast**

Do not call `save()` on every pass of `loop()`. Use `autosave(3000)` with `update()` or save on an event. For frequent data (counters) prefer RTC memory or an SD card.

**On ESP32 it writes but after reboot it is empty**

The library calls `EEPROM.begin()` and `commit()` itself. Do not call your own `EEPROM.begin()` with a smaller size or reuse addresses. `errorText()` shows ERR_WRITE_FAILED if commit failed.

### Self-check
The library has `diagnose(Serial)`: it prints what the board sees and tells what to check. Tips language: `setLanguage(LANG_RU / LANG_KK / LANG_EN)`.

### Examples
`Basic`, `Autosave`, `Migration`, `Diagnose`

---

MIT License, Alash Electronics. https://github.com/Alash-electronics/AlashSettings
