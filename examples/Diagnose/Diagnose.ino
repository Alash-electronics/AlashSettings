// AlashSettings: самопроверка. Печатает размер структуры, число ячеек, результат загрузки (первый запуск, версия, повреждение),
// число исправных ячеек, счётчик записей и подсказки. Язык: LANG_RU, LANG_KK или LANG_EN.
#include <AlashSettings.h>

struct Config { uint16_t a; float b; };

AlashSettings<Config> settings(1, 0, 4);

void setup() {
  Serial.begin(115200);
  settings.setLanguage(AlashSettingsBase::LANG_RU);
  Config defaults = {1, 2.5f};
  settings.begin(defaults);
  settings.diagnose(Serial);
}

void loop() {}
