#include "Main.h"

#include "../BetterMangaApp/models/Common.hpp"
#include "../plugins/BetterMangaApp.h"
#include "../utils/Utils.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Add definition of your processing function here
void Main::getInfo(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback) {
  auto *pluginPtr = app().getPlugin<BetterMangaApp>();

  json result;
  result["version"] = app().getCustomConfig()["version"].asString();
  result["availableDrivers"] = split(pluginPtr->getDrivers(), ",");

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setStatusCode(k200OK);
  resp->setBody(result.dump());

  callback(resp);
}

void Main::getList(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback) {
  auto parameters = req->getParameters();
  auto tryDriver = parameters.find("driver");

  if (tryDriver == parameters.end()) {

    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k400BadRequest);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody("{\"error\":\"\\\"driver\\\" is missing.\"}");

    return callback(resp);
  }

  auto tryCategory = parameters.find("category");
  auto tryPage = parameters.find("page");
  auto tryProxy = parameters.find("proxy");
  auto tryStatus = parameters.find("status");

  Category category = All;
  int page = 1;
  bool proxy = false;
  Status status = Any;

  if (tryCategory != parameters.end())
    category = stringToCategory(tryCategory->second);

  if (tryPage != parameters.end()) {
    int parsedPage = std::atoi(tryPage->second.c_str());
    if (parsedPage > 0)
      page = parsedPage;
  }

  if (tryProxy != parameters.end())
    proxy = std::atoi(tryProxy->second.c_str()) == 1;

  if (tryStatus != parameters.end()) {
    int parsedStatus = std::atoi(tryStatus->second.c_str());
    if (parsedStatus >= 0 && parsedStatus <= 2)
      status = (Status)parsedStatus;
  }

  auto *pluginPtr = app().getPlugin<BetterMangaApp>();
  callback(
      pluginPtr->getList(tryDriver->second, category, page, status, proxy));
}

void Main::getManga(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback) {

  auto parameters = req->getParameters();
  auto tryDriver = parameters.find("driver");

  bool isId = false;
  vector<string> ids;
  if (req->getMethod() == Get) {
    auto tryIds = parameters.find("ids");
    if (tryIds != parameters.end()) {
      ids = split(tryIds->second, ",");
      isId = true;
    }
  } else {
    try {
      json body = json::parse(req->getBody());
      ids = body["ids"].get<vector<string>>();
      isId = true;
    } catch (...) {
    }
  }

  if (tryDriver == parameters.end() || !isId) {
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k400BadRequest);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody("{\"error\":\"\\\"driver\\\" or \\\"ids\\\" are missing.\"}");

    return callback(resp);
  }

  auto tryProxy = parameters.find("proxy");
  auto tryShowAll = parameters.find("show-all");

  bool proxy = false;
  bool showAll = false;

  if (tryProxy != parameters.end())
    proxy = std::atoi(tryProxy->second.c_str()) == 1;

  if (tryShowAll != parameters.end())
    showAll = std::atoi(tryShowAll->second.c_str()) == 1;

  auto *pluginPtr = app().getPlugin<BetterMangaApp>();
  callback(pluginPtr->getManga(tryDriver->second, ids, showAll, proxy));
}

void Main::getChapter(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback) {
  auto parameters = req->getParameters();
  auto tryDriver = parameters.find("driver");
  auto tryId = parameters.find("id");

  if (tryDriver == parameters.end() || tryId == parameters.end()) {
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k400BadRequest);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(
        "{\"error\":\"\\\"driver\\\" or \\\"keyword\\\" are missing.\"}");

    return callback(resp);
  }

  auto tryExtraData = parameters.find("extra-data");
  auto tryProxy = parameters.find("proxy");

  string extraData;
  bool proxy = false;

  if (tryExtraData != parameters.end()) {
    extraData = tryExtraData->second;
  }

  if (tryProxy != parameters.end())
    proxy = std::atoi(tryProxy->second.c_str()) == 1;

  auto *pluginPtr = app().getPlugin<BetterMangaApp>();
  callback(pluginPtr->getChapter(tryDriver->second, tryId->second, extraData,
                                 proxy));
}

void Main::getSuggestion(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto parameters = req->getParameters();
  auto tryDriver = parameters.find("driver");
  auto tryKeyword = parameters.find("keyword");

  if (tryDriver == parameters.end() || tryKeyword == parameters.end()) {
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k400BadRequest);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(
        "{\"error\":\"\\\"driver\\\" or \\\"keyword\\\" are missing.\"}");

    return callback(resp);
  }

  auto *pluginPtr = app().getPlugin<BetterMangaApp>();
  callback(pluginPtr->getSuggestion(tryDriver->second, tryKeyword->second));
}

void Main::getSearch(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback) {
  auto parameters = req->getParameters();
  auto tryDriver = parameters.find("driver");
  auto tryKeyword = parameters.find("keyword");

  if (tryDriver == parameters.end() || tryKeyword == parameters.end()) {
    HttpResponsePtr resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k400BadRequest);
    resp->setContentTypeCode(CT_APPLICATION_JSON);
    resp->setBody(
        "{\"error\":\"\\\"driver\\\" or \\\"keyword\\\" are missing.\"}");

    return callback(resp);
  }

  auto tryPage = parameters.find("page");
  auto tryProxy = parameters.find("proxy");

  int page = 1;
  bool proxy = false;

  if (tryPage != parameters.end()) {
    int parsedPage = std::atoi(tryPage->second.c_str());
    if (parsedPage > 0)
      page = parsedPage;
  }

  if (tryProxy != parameters.end())
    proxy = std::atoi(tryProxy->second.c_str()) == 1;

  auto *pluginPtr = app().getPlugin<BetterMangaApp>();
  callback(
      pluginPtr->getSearch(tryDriver->second, tryKeyword->second, page, proxy));
}