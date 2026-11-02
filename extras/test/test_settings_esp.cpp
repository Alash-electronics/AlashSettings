// Тесты AlashSettings без железа, ESP (EEPROM.begin() и commit() обязательны): те же проверки.
//   g++ -std=c++17 -Wall -DARDUINO_ARCH_ESP32 -fsanitize=address,undefined -I mock -I ../../src test_settings_esp.cpp ../../src/AlashSettings.cpp && ./a.out
#include "settings_cases.h"
int main() { return runCases(); }
