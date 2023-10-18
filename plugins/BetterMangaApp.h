/**
 *
 *  BetterMangaApp.h
 *
 */

#pragma once

#include "../BetterMangaApp/models/BaseDriver.hpp"

#include <drogon/HttpController.h>
#include <drogon/plugins/Plugin.h>
#include <string>
#include <vector>

class BetterMangaApp : public drogon::Plugin<BetterMangaApp> {
public:
  BetterMangaApp() {}
  /// This method must be called by drogon to initialize and start the plugin.
  /// It must be implemented by the user.
  void initAndStart(const Json::Value &config) override;

  /// This method must be called by drogon to shutdown the plugin.
  /// It must be implemented by the user.
  void shutdown() override;

  string getDrivers();

  drogon::HttpResponsePtr getInfo(std::string id);

  drogon::HttpResponsePtr getProxy(std::vector<std::string> ids);

  drogon::HttpResponsePtr getOnline(std::vector<std::string> ids);

  drogon::HttpResponsePtr getList(std::string id, Category category, int page,
                                  Status status, bool useProxy);

  drogon::HttpResponsePtr getSuggestion(std::string id, std::string keyword);

  drogon::HttpResponsePtr getSearch(std::string id, std::string keyword,
                                    int page, bool useProxy);

  drogon::HttpResponsePtr getChapter(std::string driver, std::string id,
                                     string extraData, bool useProxy);

  drogon::HttpResponsePtr getManga(std::string driver,
                                   std::vector<std::string> ids, bool showAll,
                                   bool useProxy);

private:
  std::vector<BaseDriver *> drivers;

  BaseDriver *getDriver(std::string id);

  drogon::HttpResponsePtr driverNotFound();
};
