#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <locale>

#include <QApplication>

int main(int argc, char *argv[]) {
  Catch::StringMaker<double>::precision = 15;

  QApplication a(argc, argv);

  // Qt sets the locale according to the system one
  // This can cause problems with numerical code
  // e.g. when a double is "3,142" vs "3.142"
  // Here we reset the default "C" locale to avoid these issues
  std::locale::global(std::locale::classic());

  int result = Catch::Session().run(argc, argv);

  return result;
}