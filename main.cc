#include <drogon/drogon.h>

#include "utils/dotenv.h"

int main() {
  // Load config file
  dotenv::init("../.env");
  drogon::app().loadConfigFile("../config.json");

  // Run HTTP framework,the method will block in the internal event loop
  drogon::app().run();
  return 0;
}
