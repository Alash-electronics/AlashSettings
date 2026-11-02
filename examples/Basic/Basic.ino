// [ru] AlashSettings: настройки в EEPROM одной структурой. Счётчик запусков и яркость переживают выключение питания.
//      Откройте Serial Monitor (115200), перезагрузите плату несколько раз: счётчик растёт.
// [kk] AlashSettings: EEPROM-дағы баптаулар бір құрылыммен. Іске қосу есептегіші мен жарықтық қуат өшкенде сақталады.
//      Serial Monitor-ды (115200) ашып, тақтаны бірнеше рет қайта жүктеңіз: есептегіш өседі.
// [en] AlashSettings: settings in the EEPROM as one struct. The start counter and the brightness survive power-off.
//      Open the Serial Monitor (115200), reboot the board a few times: the counter grows.
#include <AlashSettings.h>

struct Config {
  uint16_t boots;          // сколько раз включали
  uint8_t brightness;      // любая ваша настройка
};
const Config DEFAULTS = {0, 128};         // значения при первом запуске

AlashSettings<Config> settings;           // версия 1, адрес 0, 4 ячейки для ротации износа

void setup() {
  Serial.begin(115200);
  bool fromMemory = settings.begin(DEFAULTS);   // загрузить или взять значения по умолчанию
  Serial.println(fromMemory ? F("Загружено из памяти") : F("Первый запуск: значения по умолчанию"));
  settings.data.boots++;                  // меняем поля как обычную структуру
  settings.save();                        // записать (если что-то изменилось)
  Serial.print(F("Запусков: ")); Serial.println(settings.data.boots);
  Serial.print(F("Яркость: ")); Serial.println(settings.data.brightness);
}

void loop() {}
