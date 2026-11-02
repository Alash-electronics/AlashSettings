// [ru] AlashSettings: переход на новую версию структуры без потери данных. В версии 1 была только яркость, в версии 2 добавилась скорость.
//      Раскомментируйте строку VERSION 2, загрузите, и старая яркость сохранится, а скорость получит значение по умолчанию.
// [kk] AlashSettings: деректерді жоғалтпай құрылымның жаңа нұсқасына көшу. 1 нұсқада тек жарықтық болды, 2 нұсқада жылдамдық қосылды.
//      VERSION 2 жолын ашып, жүктеңіз: ескі жарықтық сақталады, ал жылдамдық әдепкі мән алады.
// [en] AlashSettings: moving to a new struct version without losing data. Version 1 had only the brightness, version 2 added the speed.
//      Uncomment the VERSION 2 line, upload, and the old brightness is kept while the speed gets the default value.
#include <AlashSettings.h>

#define VERSION 1                         // поменяйте на 2 и загрузите снова

#if VERSION == 1
struct Config { uint8_t brightness; };
const uint8_t VER = 1;
const Config DEFAULTS = {100};
#else
struct Config { uint8_t brightness; uint8_t speed; };
const uint8_t VER = 2;
const Config DEFAULTS = {100, 5};

// Получает данные старой версии и заполняет новую структуру
bool migrate(uint8_t oldVersion, const uint8_t *oldData, uint16_t oldLength, void *out) {
  if (oldVersion != 1 || oldLength != 1) return false;     // умеем переносить только версию 1
  Config *c = (Config *)out;
  c->brightness = oldData[0];             // яркость сохраняем
  c->speed = 5;                           // новое поле: значение по умолчанию
  return true;
}
#endif

AlashSettings<Config> settings(VER);

void setup() {
  Serial.begin(115200);
#if VERSION == 2
  settings.onMigrate(migrate);
#endif
  settings.begin(DEFAULTS);
  Serial.print(F("Версия ")); Serial.println(VER);
  Serial.print(F("Яркость: ")); Serial.println(settings.data.brightness);
#if VERSION == 2
  Serial.print(F("Скорость: ")); Serial.println(settings.data.speed);
#endif
  if (VER == 1 && settings.loadResult() == AlashSettings<Config>::FIRST_BOOT) { settings.data.brightness = 222; settings.save(); }
  Serial.println(settings.loadResult() == AlashSettings<Config>::MIGRATED ? F("Данные перенесены") : F("Миграция не нужна"));
}

void loop() {}
