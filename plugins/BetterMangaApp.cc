/**
 *
 *  BetterMangaApp.cc
 *
 */

#include "BetterMangaApp.h"

#include "../BetterMangaApp/drivers/ActiveAdapter.cc"
#include "../BetterMangaApp/drivers/DM5.cc"
#include "../BetterMangaApp/drivers/MHG.cc"
#include "../BetterMangaApp/drivers/MHR.cc"
#include "../utils/TypeAliases.hpp"
#include "../utils/dotenv.h"

#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

using namespace drogon;
using json = nlohmann::json;

void BetterMangaApp::initAndStart(const Json::Value &config) {

  drivers.push_back(new MHR());
  drivers.push_back(new DM5());
  drivers.push_back(new ActiveAdapter(new MHG()));

  string version = app().getCustomConfig()["version"].asString();
  string drivers = getDrivers();

  app().registerPreSendingAdvice(
      [version, drivers](const drogon::HttpRequestPtr &req,
                         const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Version", version);
        resp->addHeader("Available-Drivers", drivers);
        resp->addHeader("Access-Control-Expose-Headers",
                        "Version, Available-Drivers, Is-Next");
        resp->addHeader("Access-Control-Allow-Origin", "*");
      });

  LOG_COMPACT_DEBUG << "BetterMangaApp initialized and started";
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-abstract-non-virtual-dtor"
void BetterMangaApp::shutdown() {
  for (BaseDriver *driver : drivers) {
    delete driver;
  }

  LOG_COMPACT_DEBUG << "BetterMangaApp shutdown";
}
#pragma GCC diagnostic pop

string BetterMangaApp::getDrivers() {
  string drivers;

  for (BaseDriver *driver : this->drivers)
    drivers += driver->id + ",";

  if (!drivers.empty())
    drivers.erase(drivers.size() - 1);

  return drivers;
}

HttpResponsePtr BetterMangaApp::getInfo(string id) {
  BaseDriver *driver = getDriver(id);

  if (driver == nullptr)
    return driverNotFound();

  vector<string> categories;
  for (Category category : driver->supportedCategories)
    categories.push_back(categoryToString(category));

  json info;
  info["supportedCategories"] = categories;
  info["recommendedChunkSize"] = driver->recommendedChunkSize;
  info["supportSuggestion"] = driver->supportSuggestion;
  info["version"] = driver->version;

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setStatusCode(k200OK);
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setBody(info.dump());

  return resp;
}

HttpResponsePtr BetterMangaApp::getProxy(vector<string> ids) {
  vector<BaseDriver *> drivers;

  if (ids.size() != 0) {
    for (string id : ids) {
      BaseDriver *driver = getDriver(id);
      if (driver != nullptr) {
        drivers.push_back(driver);
      }
    }
  } else {
    drivers = this->drivers;
  }

  json result = json::array();
  for (BaseDriver *driver : drivers) {
    result[driver->id] = driver->proxySettings.toJson();
  }

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setStatusCode(k200OK);
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setBody(result.dump());

  return resp;
}

HttpResponsePtr BetterMangaApp::getOnline(vector<string> ids) {
  vector<BaseDriver *> drivers;

  if (ids.size() != 0) {
    for (string id : ids) {
      BaseDriver *driver = getDriver(id);
      if (driver != nullptr) {
        drivers.push_back(driver);
      }
    }
  } else {
    drivers = this->drivers;
  }

  json result;
  vector<std::thread> threads;
  std::mutex mutex;

  auto testOnline = [&mutex, &result](BaseDriver *driver) {
    auto start = std::chrono::high_resolution_clock::now();
    bool online = driver->checkOnline();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::lock_guard<std::mutex> guard(mutex);
    result[driver->id]["online"] = online;
    result[driver->id]["latency"] =
        online ? (double)duration.count() / 1000 : 0;
  };

  for (BaseDriver *driver : drivers) {
    threads.push_back(std::thread(testOnline, driver));
  }

  // wait for threads to finish
  for (auto &th : threads) {
    th.join();
  }

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setStatusCode(k200OK);
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setBody(result.dump());

  return resp;
}

HttpResponsePtr BetterMangaApp::getList(string id, Category category, int page,
                                        Status status, bool useProxy) {

  BaseDriver *driver = getDriver(id);

  if (driver == nullptr)
    return driverNotFound();

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    vector<Manga *> mangas = driver->getList(category, page, status);
    json result = json::array();
    for (Manga *manga : mangas) {
      if (useProxy)
        manga->useProxy();

      result.push_back(manga->toJson());
    }

    resp->setStatusCode(k200OK);
    resp->setBody(result.dump());

    return resp;
  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\": \"An unexpected error occurred when trying to "
                  "get list.\"}");
  }

  return resp;
};

HttpResponsePtr BetterMangaApp::getSuggestion(string id, string keyword) {
  BaseDriver *driver = getDriver(id);

  if (driver == nullptr)
    return driverNotFound();

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    vector<string> suggestions = driver->getSuggestion(keyword);
    json result = suggestions;

    resp->setStatusCode(k200OK);
    resp->setBody(result.dump());

    return resp;
  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\": \"An unexpected error occurred when trying to "
                  "get suggestions.\"}");
  }

  return resp;
}

HttpResponsePtr BetterMangaApp::getManga(string id, vector<string> ids,
                                         bool showAll, bool useProxy) {
  BaseDriver *driver = getDriver(id);

  if (driver == nullptr)
    return driverNotFound();

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    vector<Manga *> mangas = driver->getManga(ids, showAll);
    json result = json::array();
    for (Manga *manga : mangas) {
      if (useProxy)
        manga->useProxy();

      result.push_back(manga->toJson());
    }

    resp->setStatusCode(k200OK);
    resp->setBody(result.dump());

    return resp;
  } catch (string e) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\": \"An unexpected error occurred when trying to "
                  "get manga.\"}");
  }

  return resp;
}

HttpResponsePtr BetterMangaApp::getSearch(string id, string keyword, int page,
                                          bool useProxy) {
  BaseDriver *driver = getDriver(id);

  if (driver == nullptr)
    return driverNotFound();

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    vector<Manga *> mangas = driver->search(keyword, page);
    json result = json::array();
    for (Manga *manga : mangas) {
      if (useProxy)
        manga->useProxy();

      result.push_back(manga->toJson());
    }

    resp->setStatusCode(k200OK);
    resp->setBody(result.dump());

    return resp;
  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\": \"An unexpected error occurred when trying to "
                  "search manga.\"}");
  }

  return resp;
}

HttpResponsePtr BetterMangaApp::getChapter(string driverId, string id,
                                           string extraData, bool useProxy) {
  BaseDriver *driver = getDriver(driverId);

  if (driver == nullptr)
    return driverNotFound();

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    vector<string> urls = driver->getChapter(id, extraData);
    json result = json::array();
    for (const string &manga : urls)
      result.push_back(useProxy ? driver->useProxy(manga, "manga") : manga);

    resp->setStatusCode(k200OK);
    resp->setBody(result.dump());

    return resp;
  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\": \"An unexpected error occurred when trying to "
                  "get chapters.\"}");
  }

  return resp;
}

BaseDriver *BetterMangaApp::getDriver(string id) {
  for (auto driver : drivers) {
    if (driver->id == id) {
      return driver;
    }
  }

  return nullptr;
}

HttpResponsePtr BetterMangaApp::driverNotFound() {
  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setStatusCode(k404NotFound);
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setBody("{\"error\":\"Driver not found\"}");

  return resp;
}