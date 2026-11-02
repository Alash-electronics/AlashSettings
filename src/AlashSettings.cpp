// MIT License. Alash Electronics.
#include "AlashSettings.h"

#define T(ru, kk, en) (_lang == LANG_KK ? F(kk) : _lang == LANG_EN ? F(en) : F(ru))

static const uint8_t MAGIC0 = 'A', MAGIC1 = 'S';

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define ALASH_SETTINGS_ESP 1
#endif

static uint16_t crc16(const uint8_t *p, uint16_t n, uint16_t crc = 0xFFFF) {   // CRC-16/CCITT-FALSE
  while (n--) {
    crc ^= (uint16_t)(*p++) << 8;
    for (uint8_t i = 0; i < 8; i++) crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
  }
  return crc;
}

static uint16_t storageLimit() {
#if defined(ALASH_SETTINGS_ESP)
  return 4096;
#else
  return EEPROM.length();
#endif
}

void AlashSettingsBase::putByte(uint16_t a, uint8_t v) {
  if (EEPROM.read(a) != v) EEPROM.write(a, v);   // одинаковые байты не пишем: меньше износ
}

bool AlashSettingsBase::commit() {
#if defined(ALASH_SETTINGS_ESP)
  return EEPROM.commit();
#else
  return true;
#endif
}

uint16_t AlashSettingsBase::dataCrc() const { return crc16(_data, _size); }

// Читает заголовок ячейки; ok = метка и контрольная сумма верны
bool AlashSettingsBase::readSlot(uint8_t s, uint16_t stride, uint8_t &ver, uint16_t &seq, uint16_t &len, uint16_t &crc, bool &ok) const {
  uint16_t a = slotAddr(s, stride);
  ok = false;
  if (a + HEADER > storageLimit()) return false;
  if (EEPROM.read(a) != MAGIC0 || EEPROM.read(a + 1) != MAGIC1) return false;
  ver = EEPROM.read(a + 2);
  seq = EEPROM.read(a + 3) | ((uint16_t)EEPROM.read(a + 4) << 8);
  len = EEPROM.read(a + 5) | ((uint16_t)EEPROM.read(a + 6) << 8);
  crc = EEPROM.read(a + 7) | ((uint16_t)EEPROM.read(a + 8) << 8);
  if (len == 0 || a + HEADER + len > storageLimit()) return false;
  uint16_t c = 0xFFFF;
  uint8_t hdr[5] = {ver, (uint8_t)seq, (uint8_t)(seq >> 8), (uint8_t)len, (uint8_t)(len >> 8)};
  c = crc16(hdr, 5, c);
  for (uint16_t i = 0; i < len; i++) { uint8_t b = EEPROM.read(a + HEADER + i); c = crc16(&b, 1, c); }
  ok = (c == crc);
  return true;
}

// Гасит метки всех ячеек со старым шагом (после смены размера структуры старые ячейки лежат по другим адресам)
void AlashSettingsBase::wipe(uint16_t stride) {
  for (uint8_t s = 0; s < _slots; s++) { uint16_t a = slotAddr(s, stride); if (a + 2 <= storageLimit()) { putByte(a, 0xFF); putByte(a + 1, 0xFF); } }
  commit();
}

bool AlashSettingsBase::load() {
  _err = ERR_OK;
  if ((uint32_t)_addr + totalBytes() > storageLimit()) { _err = ERR_TOO_BIG; _result = CORRUPT; return false; }
#if defined(ALASH_SETTINGS_ESP)
  uint16_t need = _addr + totalBytes();
  EEPROM.begin(need < 512 ? 512 : need);
#endif
  _begun = true;
  // Размер ячейки берём из ячейки 0: после смены структуры он мог быть другим
  uint8_t ver; uint16_t seq, len, crc; bool ok;
  uint16_t stride = HEADER + _size;
  bool any0 = readSlot(0, stride, ver, seq, len, crc, ok);
  if (any0 && len != _size) stride = HEADER + len;
  int best = -1; uint16_t bestSeq = 0; uint8_t bestVer = 0; uint16_t bestLen = 0;
  bool sawMagic = false;
  for (uint8_t s = 0; s < _slots; s++) {
    uint8_t v; uint16_t sq, l, c; bool good;
    if (!readSlot(s, stride, v, sq, l, c, good)) continue;
    sawMagic = true;
    if (!good || l != stride - HEADER) continue;
    if (best < 0 || (int16_t)(sq - bestSeq) > 0) { best = s; bestSeq = sq; bestVer = v; bestLen = l; }
  }
  if (best < 0) {
    _result = sawMagic ? CORRUPT : FIRST_BOOT;
    if (sawMagic) wipe(stride);
    _seq = 0; _slot = _slots - 1;
    return false;
  }
  _seq = bestSeq; _slot = best;
  uint16_t a = slotAddr(best, stride) + HEADER;
  if (bestVer == _version && bestLen == _size) {
    for (uint16_t i = 0; i < _size; i++) _data[i] = EEPROM.read(a + i);
    _savedCrc = dataCrc(); _lastCrc = _savedCrc; _haveSaved = true; _dirty = false;
    _result = LOADED;
    return true;
  }
  // версия или размер другие: пробуем перенести
  _slot = _slots - 1;                              // новая запись пойдёт в ячейку 0 с новым шагом
  if (_migrate) {
    uint8_t *tmp = (uint8_t *)malloc(bestLen);
    if (tmp) {
      for (uint16_t i = 0; i < bestLen; i++) tmp[i] = EEPROM.read(a + i);
      bool done = _migrate(bestVer, tmp, bestLen, _data);
      free(tmp);
      if (done) { wipe(stride); _result = MIGRATED; _haveSaved = false; save(true); return true; }
    }
  }
  wipe(stride);
  _result = VERSION_CHANGED;
  return false;
}

uint8_t AlashSettingsBase::validSlots() const {
  if (!_begun) return 0;
  uint8_t n = 0;
  for (uint8_t s = 0; s < _slots; s++) { uint8_t v; uint16_t sq, l, c; bool ok; if (readSlot(s, HEADER + _size, v, sq, l, c, ok) && ok && l == _size) n++; }
  return n;
}

bool AlashSettingsBase::changed() const {
  if (!_begun) return false;
  return !_haveSaved || dataCrc() != _savedCrc;
}

bool AlashSettingsBase::save(bool force) {
  if (!_begun) { _err = ERR_NOT_BEGUN; return false; }
  uint16_t crcNow = dataCrc();
  if (!force && _haveSaved && crcNow == _savedCrc) return true;       // нечего писать
  uint16_t stride = HEADER + _size;
  uint16_t seq = _seq + 1;
  uint8_t slot = (uint8_t)((_slot + 1) % _slots);                     // следующая ячейка по кругу
  uint16_t a = slotAddr(slot, stride);
  uint8_t hdr[5] = {_version, (uint8_t)seq, (uint8_t)(seq >> 8), (uint8_t)_size, (uint8_t)(_size >> 8)};
  uint16_t c = crc16(hdr, 5);
  c = crc16(_data, _size, c);
  // сначала данные, метка и контрольная сумма в конце: недописанная ячейка не пройдёт проверку, а прежняя остаётся целой
  for (uint16_t i = 0; i < _size; i++) putByte(a + HEADER + i, _data[i]);
  putByte(a + 2, hdr[0]); putByte(a + 3, hdr[1]); putByte(a + 4, hdr[2]); putByte(a + 5, hdr[3]); putByte(a + 6, hdr[4]);
  putByte(a + 7, (uint8_t)c); putByte(a + 8, (uint8_t)(c >> 8));
  putByte(a, MAGIC0); putByte(a + 1, MAGIC1);
  if (!commit()) { _err = ERR_WRITE_FAILED; return false; }
  // проверка записи чтением
  uint8_t v; uint16_t sq, l, cc; bool ok;
  if (!readSlot(slot, stride, v, sq, l, cc, ok) || !ok || sq != seq) { _err = ERR_WRITE_FAILED; return false; }
  _seq = seq; _slot = slot; _savedCrc = crcNow; _lastCrc = crcNow; _haveSaved = true; _dirty = false; _saves++;
  return true;
}

bool AlashSettingsBase::update() {
  if (!_begun || _quiet == 0) return false;
  uint16_t crcNow = dataCrc();
  uint32_t now = millis();
  if (crcNow != _lastCrc) { _lastCrc = crcNow; _changedAt = now; _dirty = true; return false; }   // ещё меняется: ждём тишины
  if (_dirty && (uint32_t)(now - _changedAt) >= _quiet) return save();
  return false;
}

void AlashSettingsBase::erase() {
  if (!_begun) { _err = ERR_NOT_BEGUN; return; }
  uint16_t stride = HEADER + _size;
  for (uint8_t s = 0; s < _slots; s++) { uint16_t a = slotAddr(s, stride); putByte(a, 0xFF); putByte(a + 1, 0xFF); }
  commit();
  _haveSaved = false; _seq = 0; _slot = _slots - 1;
}

const __FlashStringHelper *AlashSettingsBase::errorText() {
  switch (_err) {
    case ERR_OK: return F("OK");
    case ERR_NOT_BEGUN:
      return T("Хранилище не запущено: вызовите settings.begin() в setup().", "Қойма іске қосылмаған: setup() ішінде settings.begin() шақырыңыз.", "The storage is not started: call settings.begin() in setup().");
    case ERR_TOO_BIG:
      return T("Настройки не помещаются в EEPROM: структура умножается на число ячеек. Уменьшите структуру или число ячеек (третий параметр), либо сдвиньте адрес. У Uno всего 1024 байта.", "Баптаулар EEPROM-ға сыймайды: құрылым ұяшықтар санына көбейтіледі. Құрылымды немесе ұяшықтар санын (үшінші параметр) азайтыңыз. Uno-да барлығы 1024 байт.", "The settings do not fit in the EEPROM: the structure is multiplied by the number of slots. Reduce the structure or the slot count (third parameter) or move the address. Uno has only 1024 bytes.");
    case ERR_BAD_PARAM:
      return T("Неверный параметр.", "Параметр қате.", "Invalid parameter.");
    case ERR_WRITE_FAILED:
      return T("Запись не подтвердилась при проверке чтением. EEPROM мог износиться или на ESP не удался commit(). Прежние данные остались целы.", "Жазу оқып тексергенде расталмады. EEPROM тозған болуы мүмкін немесе ESP-де commit() сәтсіз. Бұрынғы деректер сақталды.", "The write failed read-back verification. The EEPROM may be worn out or commit() failed on ESP. The previous data is intact.");
  }
  return F("");
}

void AlashSettingsBase::diagnose(Print &out) {
  out.println(T("AlashSettings: диагностика", "AlashSettings: диагностика", "AlashSettings: diagnostics"));
  out.print(T("Размер структуры, байт: ", "Құрылым өлшемі, байт: ", "Structure size, bytes: ")); out.print((long)_size);
  out.print(T("   версия: ", "   нұсқа: ", "   version: ")); out.print((long)_version);
  out.print(T("   адрес: ", "   мекенжай: ", "   address: ")); out.println((long)_addr);
  out.print(T("Ячеек: ", "Ұяшықтар: ", "Slots: ")); out.print((long)_slots);
  out.print(T("   занято EEPROM, байт: ", "   EEPROM алады, байт: ", "   EEPROM used, bytes: ")); out.print((long)totalBytes());
  out.print(T(" из ", " ішінен ", " of ")); out.println((long)storageLimit());
  switch (_result) {
    case NOT_LOADED: out.println(T("Результат: begin() ещё не вызывался.", "Нәтиже: begin() әлі шақырылмаған.", "Result: begin() has not been called.")); break;
    case LOADED: out.println(T("Результат: настройки загружены из памяти.", "Нәтиже: баптаулар жадтан жүктелді.", "Result: settings loaded from memory.")); break;
    case FIRST_BOOT: out.println(T("Результат: первый запуск, взяты значения по умолчанию.", "Нәтиже: алғашқы іске қосу, әдепкі мәндер алынды.", "Result: first start, defaults were used.")); break;
    case VERSION_CHANGED: out.println(T("Результат: версия или размер структуры изменились, взяты значения по умолчанию. Чтобы сохранить старые данные, задайте onMigrate().", "Нәтиже: құрылым нұсқасы немесе өлшемі өзгерді, әдепкі мәндер алынды. Ескі деректерді сақтау үшін onMigrate() беріңіз.", "Result: the structure version or size changed, defaults were used. To keep old data provide onMigrate().")); break;
    case CORRUPT: out.println(T("Результат: данные повреждены (не сошлась контрольная сумма), взяты значения по умолчанию. Это бывает при обрыве питания во время записи на старой версии или если эти ячейки EEPROM использует другой код.", "Нәтиже: деректер бүлінген (бақылау сомасы сәйкес емес), әдепкі мәндер алынды. Басқа код осы EEPROM ұяшықтарын қолданса осылай болады.", "Result: the data is corrupt (checksum mismatch), defaults were used. This happens if other code uses the same EEPROM cells.")); break;
    case MIGRATED: out.println(T("Результат: данные старой версии перенесены.", "Нәтиже: ескі нұсқа деректері көшірілді.", "Result: data of the old version was migrated.")); break;
  }
  out.print(T("Исправных ячеек: ", "Дұрыс ұяшықтар: ", "Valid slots: ")); out.print((long)validSlots());
  out.print(T("   счётчик записей: ", "   жазулар есептегіші: ", "   write counter: ")); out.print((long)_seq);
  out.print(T("   последняя ячейка: ", "   соңғы ұяшық: ", "   last slot: ")); out.println((long)_slot);
  out.print(T("Записей с запуска: ", "Іске қосудан бері жазулар: ", "Writes since start: ")); out.println((long)_saves);
  out.print(T("Изменено и не сохранено: ", "Өзгертілген, сақталмаған: ", "Changed and not saved: ")); out.println(changed() ? T("да", "иә", "yes") : T("нет", "жоқ", "no"));
  out.println(T("Ресурс EEPROM около 100 000 записей на ячейку; ротация по ячейкам умножает его на число ячеек. Не вызывайте save() в каждом проходе loop(): используйте autosave() или сохраняйте по событию.", "EEPROM ресурсы ұяшыққа шамамен 100 000 жазу; ұяшықтар бойынша айналу оны ұяшықтар санына көбейтеді. save() әр loop() сайын шақырмаңыз: autosave() қолданыңыз.", "EEPROM endurance is about 100,000 writes per cell; slot rotation multiplies it by the slot count. Do not call save() on every pass of loop(): use autosave() or save on an event."));
  if (_err != ERR_OK) { out.print(T("Последняя ошибка: ", "Соңғы қате: ", "Last error: ")); out.println(errorText()); }
}
