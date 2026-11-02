// Имитация EEPROM: массив байтов, счётчик записей по ячейкам, обрыв питания после N записанных байтов.
// Для ESP (ARDUINO_ARCH_ESP32 / ARDUINO_ARCH_ESP8266) требует begin() и commit(), как настоящая.
#pragma once
#include "Arduino.h"
struct EEPROMClass {
  uint8_t mem[1024]; uint32_t writes[1024]; uint32_t totalWrites = 0;
  long failAfter = -1;                // сколько байтов ещё можно записать до «обрыва питания»; -1 без обрыва
  bool began = false, dirty = false, misuse = false;
  bool failCommit = false;
  EEPROMClass() { clear(); }
  void clear() { memset(mem, 0xFF, sizeof mem); memset(writes, 0, sizeof writes); totalWrites = 0; failAfter = -1; began = dirty = misuse = failCommit = false; }
  uint16_t length() const { return 1024; }
  bool begin(size_t n) { began = true; return n <= 4096; }
  uint8_t read(int a) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    if (!began) misuse = true;
#endif
    return a >= 0 && a < 1024 ? mem[a] : 0xFF;
  }
  void write(int a, uint8_t v) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    if (!began) misuse = true;
#endif
    if (a < 0 || a >= 1024) return;
    if (failAfter == 0) return;      // питание пропало: запись не доходит
    if (failAfter > 0) failAfter--;
    mem[a] = v; writes[a]++; totalWrites++; dirty = true;
  }
  bool commit() { dirty = false; return !failCommit; }
};
inline EEPROMClass EEPROM;
