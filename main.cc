#include <drogon/drogon.h>

#include <fstream>
#include <re2/re2.h>
#include <string>

int main() {
  // Load config file
  std::string configPaths[] = {"../config.json", "config.json"};
  for (const std::string &path : configPaths) {
    std::ifstream configStream(path);

    if (configStream.good()) {
      Json::Value config;
      Json::Reader reader;
      reader.parse(configStream, config);

      std::string dbPath = path;
      RE2::GlobalReplace(&dbPath, "config.json", "db.sqlite3");

      if (config.isMember("db_clients")) {
        for (auto &client : config["db_clients"]) {
          if (client["rdbms"].asString() == "sqlite3") {
            client["filename"] = dbPath;
          }
        }
      }

      drogon::app().loadConfigJson(config);

      configStream.close();
      break;
    }

    configStream.close();
  }

  // Run HTTP framework,the method will block in the internal event loop
  drogon::app().run();
  return 0;
}
