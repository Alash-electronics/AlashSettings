// Общие проверки AlashSettings; подключаются из test_settings*.cpp (AVR и ESP, у ESP нужны begin() и commit()).
#include <stdio.h>
#include "Arduino.h"
#include "EEPROM.h"
#include "AlashSettings.h"

static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); fails++; } } while (0)
typedef AlashSettingsBase B;

struct Cfg { int16_t brightness; uint8_t mode; float gain; char name[8]; };
static const Cfg DEF = {50, 2, 1.5f, "alash"};
static bool same(const Cfg &a, const Cfg &b) { return memcmp(&a, &b, sizeof(Cfg)) == 0; }
static Cfg mk(int16_t br, uint8_t mode, float gain, const char *n) { Cfg c; memset(&c, 0, sizeof c); c.brightness = br; c.mode = mode; c.gain = gain; strncpy(c.name, n, 7); return c; }
static const Cfg DEFZ = mk(50, 2, 1.5f, "alash");

// версия 1 и 2 одной настройки, для миграции
struct CfgV1 { int16_t level; };
struct CfgV2 { int16_t level; float scale; uint8_t flag; };
static bool migrate12(uint8_t oldVer, const uint8_t *old, uint16_t len, void *out) {
  if (oldVer != 1 || len != sizeof(CfgV1)) return false;
  CfgV1 o; memcpy(&o, old, sizeof o);
  CfgV2 n; n.level = o.level; n.scale = 2.0f; n.flag = 1;
  memcpy(out, &n, sizeof n);
  return true;
}

static int runCases() {
  // 1) первый запуск: значения по умолчанию, сохранены, потом загружаются
  {
    EEPROM.clear();
    AlashSettings<Cfg> s(1, 0, 4);
    CHECK(!s.begin(DEFZ) && s.loadResult() == B::FIRST_BOOT && s.wasReset());
    CHECK(same(s.data, DEFZ) && s.sequence() == 1 && s.slot() == 0 && s.saves() == 1 && s.validSlots() == 1);
    CHECK(!s.changed());
    AlashSettings<Cfg> t(1, 0, 4);
    CHECK(t.begin(mk(1, 1, 1, "x")) && t.loadResult() == B::LOADED && !t.wasReset());
    CHECK(same(t.data, DEFZ));                                     // значения из памяти, а не переданные defaults
  }

  // 2) изменение, changed(), сохранение, повторное сохранение без изменений ничего не пишет
  {
    EEPROM.clear(); AlashSettings<Cfg> s(1); s.begin(DEFZ);
    s.data.brightness = 80; CHECK(s.changed());
    CHECK(s.save() && !s.changed() && s.sequence() == 2 && s.slot() == 1);
    uint32_t w = EEPROM.totalWrites; CHECK(s.save() && EEPROM.totalWrites == w && s.sequence() == 2);   // без изменений не пишет
    CHECK(s.saveNow() && s.sequence() == 3);                                                           // а принудительно пишет
    AlashSettings<Cfg> t(1); t.begin(DEFZ); CHECK(t.data.brightness == 80 && t.sequence() == 3);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    CHECK(!EEPROM.misuse && !EEPROM.dirty);                         // begin() вызван до чтения, commit() после записи
#endif
  }

  // 3) ротация ячеек: износ делится между слотами
  {
    EEPROM.clear(); AlashSettings<Cfg> s(1, 0, 4); s.begin(DEFZ);
    for (int i = 0; i < 1000; i++) { s.data.brightness = i; s.data.gain = i * 0.5f; s.save(); }
    uint32_t mx = 0; for (int i = 0; i < 4 * (9 + sizeof(Cfg)); i++) if (EEPROM.writes[i] > mx) mx = EEPROM.writes[i];
    CHECK(mx <= 1000 / 4 + 5);                                       // без ротации любая ячейка писалась бы 1000 раз
    AlashSettings<Cfg> t(1, 0, 4); t.begin(DEFZ); CHECK(t.data.brightness == 999 && t.sequence() == 1001);
  }

  // 4) обрыв питания в любой момент записи: данные либо старые, либо новые, но никогда не мусор
  {
    int cuts = 0, old = 0, neu = 0, bad = 0;
    for (long cut = 0; cut < 60; cut++) {
      EEPROM.clear(); AlashSettings<Cfg> s(1, 0, 3); s.begin(DEFZ);
      s.data = mk(111, 3, 2.5f, "first"); s.save();
      Cfg before = s.data;
      EEPROM.failAfter = cut;
      s.data = mk(222, 4, 3.5f, "second"); s.save();
      EEPROM.failAfter = -1;
      AlashSettings<Cfg> t(1, 0, 3); bool ok = t.begin(mk(9, 9, 9, "dflt"));
      cuts++;
      if (!ok) bad++;
      else if (same(t.data, before)) old++;
      else if (same(t.data, mk(222, 4, 3.5f, "second"))) neu++;
      else bad++;
    }
    CHECK(bad == 0 && old > 0 && neu > 0 && cuts == 60);
  }

  // 5) порча: испорчена новая ячейка, берётся прежняя; испорчены все, берутся значения по умолчанию
  {
    EEPROM.clear(); AlashSettings<Cfg> s(1, 0, 4); s.begin(DEFZ);
    s.data.brightness = 70; s.save(); s.data.brightness = 90; s.save();      // слоты 0, 1, 2
    int newest = s.slot(); uint16_t a = newest * (9 + sizeof(Cfg)) + 9;
    EEPROM.mem[a] ^= 0x55;
    AlashSettings<Cfg> t(1, 0, 4); CHECK(t.begin(DEFZ) && t.data.brightness == 70 && t.validSlots() == 2);   // откат на предыдущую
    for (int i = 0; i < 4 * (9 + sizeof(Cfg)); i += 3) EEPROM.mem[i] ^= 0xA5;
    AlashSettings<Cfg> u(1, 0, 4); CHECK(!u.begin(DEFZ) && u.loadResult() == B::CORRUPT && same(u.data, DEFZ));
    AlashSettings<Cfg> v(1, 0, 4); v.begin(DEFZ); CHECK(v.loadResult() == B::LOADED);   // после починки (u сохранил defaults) снова работает
  }

  // 6) смена версии: без миграции значения по умолчанию, с миграцией данные переносятся (размер структуры тоже другой)
  {
    EEPROM.clear(); AlashSettings<CfgV1> old(1, 0, 4); CfgV1 d1 = {42}; old.begin(d1); old.data.level = 77; old.save();
    AlashSettings<CfgV2> plain(2, 0, 4); CfgV2 d2 = {5, 1.0f, 0};
    CHECK(!plain.begin(d2) && plain.loadResult() == B::VERSION_CHANGED && plain.data.level == 5);
    EEPROM.clear(); AlashSettings<CfgV1> old2(1, 0, 4); old2.begin(d1); old2.data.level = 77; old2.save();
    AlashSettings<CfgV2> mig(2, 0, 4); mig.onMigrate(migrate12);
    CHECK(mig.begin(d2) && mig.loadResult() == B::MIGRATED && mig.data.level == 77 && mig.data.scale == 2.0f && mig.data.flag == 1);
    AlashSettings<CfgV2> again(2, 0, 4); CHECK(again.begin(d2) && again.loadResult() == B::LOADED && again.data.level == 77);   // перенос сохранился
    // размер изменился, а версию забыли поднять: данные не принимаются как есть
    EEPROM.clear(); AlashSettings<CfgV1> o3(1, 0, 2); o3.begin(d1); o3.data.level = 5; o3.save();
    AlashSettings<CfgV2> forgot(1, 0, 2); CHECK(!forgot.begin(d2) && forgot.loadResult() == B::VERSION_CHANGED);
  }

  // 7) не помещается; два набора настроек по разным адресам не мешают друг другу
  {
    EEPROM.clear();
    struct Big { uint8_t x[1100]; };
    AlashSettings<Big> big(1, 0, 4); CHECK(!big.begin() && big.error() == B::ERR_TOO_BIG && !big.save());
    AlashSettings<Cfg> a(1, 0, 2); AlashSettings<CfgV1> b(1, 100, 2);
    CfgV1 d1 = {1}; a.begin(DEFZ); b.begin(d1);
    a.data.brightness = 11; a.save(); b.data.level = 22; b.save();
    AlashSettings<Cfg> a2(1, 0, 2); AlashSettings<CfgV1> b2(1, 100, 2); a2.begin(DEFZ); b2.begin(d1);
    CHECK(a2.data.brightness == 11 && b2.data.level == 22);
  }

  // 8) автосохранение с ожиданием тишины
  {
    EEPROM.clear(); AlashSettings<Cfg> s(1); s.begin(DEFZ); s.autosave(2000);
    uint32_t saves = s.saves();
    s.data.brightness = 10; g_ms += 100; CHECK(!s.update());       // заметили изменение, ждём
    for (int i = 0; i < 15; i++) { g_ms += 500; s.data.brightness = 20 + i; CHECK(!s.update()); }   // меняется постоянно: не пишем
    CHECK(s.saves() == saves);
    g_ms += 1900; CHECK(!s.update());
    g_ms += 200; CHECK(s.update() && s.saves() == saves + 1 && !s.changed());       // тишина 2 секунды: записали
    g_ms += 5000; CHECK(!s.update());                              // без изменений больше не пишет
    s.autosave(0); s.data.mode = 9; g_ms += 9000; CHECK(!s.update() && s.changed());   // автосохранение выключено
  }

  // 9) стирание, перезапись, счётчик через переполнение 16 бит
  {
    EEPROM.clear(); AlashSettings<Cfg> s(1, 0, 3); s.begin(DEFZ); s.data.mode = 7; s.save();
    s.erase(); AlashSettings<Cfg> t(1, 0, 3); CHECK(!t.begin(DEFZ) && t.loadResult() == B::FIRST_BOOT);
    EEPROM.clear(); AlashSettings<Cfg> w(1, 0, 3); w.begin(DEFZ);
    for (long i = 0; i < 70000; i++) { w.data.brightness = (int16_t)(i & 0x3FFF); w.save(); }
    AlashSettings<Cfg> r(1, 0, 3); r.begin(DEFZ);
    CHECK(r.data.brightness == (int16_t)(69999 & 0x3FFF) && r.sequence() == (uint16_t)(70001));    // выбрана самая новая ячейка через переход счётчика
  }

  // 10) сбои записи и вызовы до begin()
  {
    EEPROM.clear(); AlashSettings<Cfg> s(1);
    CHECK(!s.save() && s.error() == B::ERR_NOT_BEGUN && !s.changed() && !s.update());
    s.erase(); CHECK(s.error() == B::ERR_NOT_BEGUN);
    s.clearError(); s.begin(DEFZ);
    s.data.mode = 5; EEPROM.failAfter = 3; CHECK(!s.save() && s.error() == B::ERR_WRITE_FAILED); EEPROM.failAfter = -1;
    CHECK(s.changed());                                            // не записалось: изменения остаются несохранёнными
    s.clearError(); CHECK(s.save() && !s.changed());
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    s.data.mode = 6; EEPROM.failCommit = true; CHECK(!s.save() && s.error() == B::ERR_WRITE_FAILED); EEPROM.failCommit = false;
#endif
  }

  // 11) тексты и диагностика
  {
    EEPROM.clear(); AlashSettings<Cfg> s(1);
    for (int e = B::ERR_NOT_BEGUN; e <= B::ERR_WRITE_FAILED; e++) {
      if (e == B::ERR_BAD_PARAM) continue;
      s.clearError();
      if (e == B::ERR_NOT_BEGUN) s.save();
      else if (e == B::ERR_TOO_BIG) { struct Big { uint8_t x[1100]; }; AlashSettings<Big> b(1, 0, 4); b.begin(); s.setLanguage(B::LANG_RU); CHECK(b.error() == B::ERR_TOO_BIG); }
      else { s.begin(DEFZ); EEPROM.failAfter = 1; s.data.mode = 3; s.save(); EEPROM.failAfter = -1; }
    }
    s.setLanguage(B::LANG_RU); const char *ru = (const char *)s.errorText();
    s.setLanguage(B::LANG_KK); const char *kk = (const char *)s.errorText();
    s.setLanguage(B::LANG_EN); const char *en = (const char *)s.errorText();
    CHECK(s.error() == B::ERR_WRITE_FAILED && strlen(ru) > 20 && strlen(kk) > 20 && strlen(en) > 20 && strcmp(ru, kk) && strcmp(kk, en));
    s.clearError(); CHECK(strcmp((const char *)s.errorText(), "OK") == 0);
    EEPROM.clear(); AlashSettings<Cfg> d(3, 16, 5); d.setLanguage(B::LANG_EN); d.begin(DEFZ);
    Print p; d.diagnose(p);
    CHECK(p.text.find("first start") != std::string::npos && p.text.find("Slots: 5") != std::string::npos && p.text.find("address: 16") != std::string::npos);
    d.setLanguage(B::LANG_KK); Print pk; d.diagnose(pk); CHECK(pk.text.find("Ұяшықтар: 5") != std::string::npos);
    AlashSettings<Cfg> n(1); Print pn; n.diagnose(pn); CHECK(pn.text.find("begin()") != std::string::npos);
  }

  if (fails) { printf("FAILED: %d\n", fails); return 1; }
  printf("ALL TESTS PASSED\n");
  return 0;
}
