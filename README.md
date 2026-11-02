# AlashSettings

[Русский](#русский) | [Қазақша](#қазақша) | [English](#english)

---

## Русский

**Настройки, которые переживают выключение**, одной структурой: `settings.data.volume = 7; settings.save();`. Контрольная сумма, версия, значения по умолчанию, **ротация ячеек** (EEPROM служит в разы дольше), **защита от обрыва питания во время записи**, перенос данных со старой версии и автосохранение. Работает на Arduino, ESP8266 и ESP32. Без зависимостей.

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

### Чем отличается от других библиотек
Мы изучили EEPROM_Rotate, EEPROMWearLevel, EEManager (GyverLibs), Preferences (ESP32) и сам EEPROM.put/get:
- Сам EEPROM.put/get не знает ни версии, ни контрольной суммы, ни первого запуска: после смены структуры читает мусор; у нас версии, CRC и defaults.
- EEPROMWearLevel и EEPROM_Rotate делят износ, но без версий и миграции; у нас всё вместе плюс защита от обрыва питания.
- EEManager хорош, но без миграции со сменой размера и на русском только в документации; у нас сообщения и подсказки на ru/kk/en.
- Preferences есть только на ESP32 (NVS); у нас один код для AVR, ESP8266 и ESP32.

### Примеры
`Basic`, `Autosave`, `Migration`, `Diagnose`

### Тесты
Проверено без железа (с AddressSanitizer, в режимах AVR, ESP32 и ESP8266): первый запуск и загрузка, ротация (износ в 4 раза ниже), **обрыв питания на каждом байте записи**, порча новой и всех ячеек, смена версии и размера, миграция, два набора, автосохранение, переход счётчика через 65535, сбои записи и commit, ошибки и диагностика: `extras/test`.

Подробное руководство: [docs/WIKI.md](docs/WIKI.md).

---

## Қазақша

**Өшіргенде сақталатын баптаулар** бір құрылыммен: `settings.data.volume = 7; settings.save();`. Бақылау сомасы, нұсқа, әдепкі мәндер, **ұяшықтар айналымы** (EEPROM есе ұзақ қызмет етеді), **жазу кезінде қуат үзілуінен қорғау**, ескі нұсқадан деректерді көшіру және автосақтау. Arduino, ESP8266 және ESP32 үшін. Тәуелділіксіз.

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

### Басқа кітапханалардан айырмашылығы
EEPROM_Rotate, EEPROMWearLevel, EEManager (GyverLibs), Preferences (ESP32) және EEPROM.put/get қарадық:
- EEPROM.put/get нұсқаны, бақылау сомасын, алғашқы іске қосуды білмейді: құрылым өзгергеннен кейін қоқыс оқиды; бізде нұсқа, CRC және defaults бар.
- EEPROMWearLevel мен EEPROM_Rotate тозуды бөледі, бірақ нұсқасыз және көшірусіз; бізде бәрі бірге және қуат үзілуінен қорғау.
- EEManager жақсы, бірақ өлшем өзгерген көшіруі жоқ; бізде ru/kk/en хабарлар.
- Preferences тек ESP32-де; бізде AVR, ESP8266 және ESP32 үшін бір код.

### Мысалдар
`Basic`, `Autosave`, `Migration`, `Diagnose`

### Тесттер
Құрылғысыз тексерілген (AddressSanitizer-мен, AVR, ESP32 және ESP8266 режимдерінде): алғашқы іске қосу, ұяшықтар айналымы (тозу 4 есе аз), **жазудың әр байтында қуат үзілуі**, жаңа және барлық ұяшықтың бүлінуі, нұсқа мен өлшем өзгерісі, көшіру, автосақтау, 65535 асуы, жазу ақаулары: `extras/test`.

Толық нұсқаулық: [docs/WIKI.md](docs/WIKI.md).

---

## English

**Settings that survive power-off** in one struct: `settings.data.volume = 7; settings.save();`. Checksum, version, defaults, **slot rotation** (the EEPROM lasts many times longer), **protection from power loss during a write**, migration from an old version and autosave. Works on Arduino, ESP8266 and ESP32. No dependencies.

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

### How it differs from other libraries
We studied EEPROM_Rotate, EEPROMWearLevel, EEManager (GyverLibs), Preferences (ESP32) and plain EEPROM.put/get:
- Plain EEPROM.put/get knows no version, checksum or first start: after a struct change it reads garbage; ours has versions, CRC and defaults.
- EEPROMWearLevel and EEPROM_Rotate share the wear but have no versions or migration; ours does everything plus power-loss protection.
- EEManager is good but has no migration across a size change; ours has messages and tips in ru/kk/en.
- Preferences exists only on ESP32 (NVS); ours is one code for AVR, ESP8266 and ESP32.

### Examples
`Basic`, `Autosave`, `Migration`, `Diagnose`

### Tests
Checked without hardware (with AddressSanitizer, in AVR, ESP32 and ESP8266 modes): first start and load, rotation (wear 4 times lower), **power loss at every byte of a write**, corruption of the newest and of all slots, version and size change, migration, two sets, autosave, counter wrap at 65535, write and commit failures, errors and diagnostics: `extras/test`.

Full guide: [docs/WIKI.md](docs/WIKI.md).

---

MIT License, Alash Electronics.
