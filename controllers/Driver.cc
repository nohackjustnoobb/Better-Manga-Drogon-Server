#include "Driver.h"

#include "../plugins/BetterMangaApp.h"
#include "../utils/Utils.hpp"

#include <drogon/HttpController.h>

// Add definition of your processing function here
void Driver::getDriver(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {

  auto parameters = req->getParameters();
  auto tryDriver = parameters.find("driver");

  if (tryDriver != parameters.end()) {
    auto *pluginPtr = app().getPlugin<BetterMangaApp>();
    callback(pluginPtr->getInfo(tryDriver->second));

    return;
  }

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setStatusCode(k400BadRequest);
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setBody("{\"error\":\"\\\"driver\\\" is missing.\"}");

  callback(resp);
}

void Driver::getProxy(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback) {
  auto parameters = req->getParameters();
  auto tryDrivers = parameters.find("drivers");

  std::vector<std::string> ids;

  if (tryDrivers != parameters.end())
    ids = split(tryDrivers->second, RE2(","));

  auto *pluginPtr = app().getPlugin<BetterMangaApp>();
  callback(pluginPtr->getProxy(ids));
}

void Driver::getOnline(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto parameters = req->getParameters();
  auto tryDrivers = parameters.find("drivers");

  std::vector<std::string> ids;

  if (tryDrivers != parameters.end())
    ids = split(tryDrivers->second, RE2(","));

  auto *pluginPtr = app().getPlugin<BetterMangaApp>();
  callback(pluginPtr->getOnline(ids));
}