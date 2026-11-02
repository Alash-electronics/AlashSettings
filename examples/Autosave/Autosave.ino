// AlashSettings: автосохранение. Значение меняется постоянно (как положение ручки), а запись в EEPROM идёт только через 3 секунды
// после последнего изменения, поэтому память не изнашивается. Крутите потенциометр на A0, в Serial Monitor (115200) видно записи.
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
