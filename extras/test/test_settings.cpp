// Тесты AlashSettings без железа, AVR (EEPROM.update-подобная запись, commit не нужен): имитация EEPROM с обрывом питания.
//   g++ -std=c++17 -Wall -fsanitize=address,undefined -I mock -I ../../src test_settings.cpp ../../src/AlashSettings.cpp && ./a.out
#include "settings_cases.h"
int main() { return runCases(); }
