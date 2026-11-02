// [ru] AlashSettings: автосохранение. Значение меняется постоянно (как положение ручки), а запись в EEPROM идёт только через 3 секунды
//      после последнего изменения, поэтому память не изнашивается. Крутите потенциометр на A0, в Serial Monitor (115200) видно записи.
// [kk] AlashSettings: автосақтау. Мән үнемі өзгереді (тұтқа орны сияқты), ал EEPROM-ға жазу соңғы өзгерістен кейін тек 3 секундтан соң жүреді,
//      сондықтан жад тозбайды. A0-дегі потенциометрді бұраңыз, Serial Monitor (115200) жазуларды көрсетеді.
// [en] AlashSettings: autosave. The value changes constantly (like a knob position), but the write to the EEPROM happens only 3 seconds
//      after the last change, so the memory does not wear out. Turn the potentiometer on A0, the Serial Monitor (115200) shows the writes.
#include <AlashSettings.h>

struct Config {
  uint16_t level;
};

AlashSettings<Config> settings;

void setup() {
  Serial.begin(115200);
  Config defaults = {0};
  settings.begin(defaults);
  settings.autosave(3000);                // сохранять через 3 с после последнего изменения
  Serial.print(F("Сохранённый уровень: ")); Serial.println(settings.data.level);
}

void loop() {
  settings.data.level = analogRead(A0);   // меняется часто
  if (settings.update()) {                // записало в EEPROM (раз за тишину)
    Serial.print(F("Сохранено: ")); Serial.print(settings.data.level);
    Serial.print(F(", записей с запуска: ")); Serial.println(settings.saves());
  }
  delay(20);
}
