// AlashSettings: настройки в EEPROM одной структурой. Контрольная сумма, версия, значения по умолчанию, ротация ячеек (износ ниже
// в несколько раз), защита от обрыва питания, миграция со старой версии и отложенное автосохранение. Без зависимостей.
// AVR, ESP8266 и ESP32 (через встроенную EEPROM.h). MIT License. Alash Electronics, https://github.com/Alash-electronics/AlashSettings
#ifndef ALASH_SETTINGS_H
#define ALASH_SETTINGS_H

#include <Arduino.h>
#include <EEPROM.h>

class AlashSettingsBase {
 public:
  // Что произошло при begin()
  enum LoadResult : uint8_t { NOT_LOADED = 0, LOADED, FIRST_BOOT, VERSION_CHANGED, CORRUPT, MIGRATED };
  enum Error : uint8_t { ERR_OK = 0, ERR_NOT_BEGUN, ERR_TOO_BIG, ERR_BAD_PARAM, ERR_WRITE_FAILED };
  enum Language : uint8_t { LANG_RU = 0, LANG_KK = 1, LANG_EN = 2 };
  // Перенос данных старой версии: получает старую версию и байты, должна заполнить новую структуру и вернуть true
  typedef bool (*Migrate)(uint8_t oldVersion, const uint8_t *oldData, uint16_t oldLength, void *newData);

  bool save(bool force = false);                  // записать, если что-то изменилось (или force)
  bool saveNow() { return save(true); }
  bool changed() const;                           // отличаются ли данные в памяти от сохранённых
  void erase();                                   // стереть все ячейки (после этого begin() даст значения по умолчанию)
  void autosave(uint32_t quietMs) { _quiet = quietMs; }   // сохранять через quietMs после последнего изменения; 0 = выключить
  bool update();                                  // вызывайте в loop(): выполняет автосохранение; true, если записало
  void onMigrate(Migrate fn) { _migrate = fn; }

  LoadResult loadResult() const { return _result; }
  bool wasReset() const { return _result == FIRST_BOOT || _result == VERSION_CHANGED || _result == CORRUPT; }
  uint8_t validSlots() const;                     // сколько ячеек сейчас исправны
  uint16_t sequence() const { return _seq; }      // сколько раз сохранялось (счётчик ячеек)
  uint8_t slot() const { return _slot; }          // в какую ячейку писали в последний раз
  uint32_t saves() const { return _saves; }       // сколько записей сделано с запуска
  uint16_t totalBytes() const { return _slots * (uint16_t)(HEADER + _size); }

  Error error() const { return _err; }
  void clearError() { _err = ERR_OK; }
  void setLanguage(Language lang) { _lang = lang; }
  const __FlashStringHelper *errorText();
  void diagnose(Print &out);

  static const uint8_t HEADER = 9;                // метка 2 + версия 1 + номер 2 + длина 2 + контрольная сумма 2

 protected:
  AlashSettingsBase(void *data, uint16_t size, uint8_t version, uint16_t address, uint8_t slots)
      : _data((uint8_t *)data), _size(size), _version(version), _addr(address), _slots(slots) {}
  bool load();                                    // подготовить хранилище и прочитать; true, если данные взяты из памяти
  void applyDefaults() { _result = _result == NOT_LOADED ? FIRST_BOOT : _result; }
  uint8_t *_data; uint16_t _size; uint8_t _version; uint16_t _addr; uint8_t _slots;
  LoadResult _result = NOT_LOADED;

 private:
  uint16_t slotAddr(uint8_t s, uint16_t stride) const { return _addr + (uint16_t)s * stride; }
  bool readSlot(uint8_t s, uint16_t stride, uint8_t &ver, uint16_t &seq, uint16_t &len, uint16_t &crc, bool &ok) const;
  uint16_t dataCrc() const;
  void wipe(uint16_t stride);
  void putByte(uint16_t a, uint8_t v);
  bool commit();
  Migrate _migrate = nullptr;
  uint16_t _seq = 0, _savedCrc = 0, _lastCrc = 0; uint8_t _slot = 0; bool _begun = false, _haveSaved = false, _dirty = false;
  uint32_t _saves = 0, _quiet = 0, _changedAt = 0;
  Error _err = ERR_OK; Language _lang = LANG_RU;
};

template <typename T>
class AlashSettings : public AlashSettingsBase {
 public:
  T data;                                         // ваши настройки: читайте и меняйте поля как обычно
  // version: меняйте при изменении структуры; address: начало в EEPROM; slots: на сколько ячеек делить износ (1..16)
  explicit AlashSettings(uint8_t version = 1, uint16_t address = 0, uint8_t slots = 4)
      : AlashSettingsBase(&data, sizeof(T), version, address, slots < 1 ? 1 : (slots > 16 ? 16 : slots)), data() {}
  // Загрузить; при первом запуске, смене версии или порче данных взять defaults (и сохранить). true, если данные взяты из памяти.
  bool begin(const T &defaults) {
    bool ok = load();
    if (!ok) { data = defaults; applyDefaults(); if (error() != ERR_TOO_BIG) save(true); }
    return ok;
  }
  bool begin() { T zero = T(); return begin(zero); }
  void reset(const T &defaults) { data = defaults; save(true); }
};

#endif
