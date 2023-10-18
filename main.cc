#include <drogon/drogon.h>
#include <fstream>
#include <string>

int main() {
  // Load config file
  std::string configPaths[] = {"../config.json", "config.json"};
  for (const std::string &path : configPaths) {
    if (FILE *file = fopen(path.c_str(), "r")) {
      fclose(file);
      drogon::app().loadConfigFile(path);
    }
  }

  // Run HTTP framework,the method will block in the internal event loop
  drogon::app().run();
  return 0;
}
