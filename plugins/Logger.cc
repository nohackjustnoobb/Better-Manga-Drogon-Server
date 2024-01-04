/**
 *
 *  Logger.cc
 *
 */

#include "Logger.h"
#include <drogon/HttpController.h>
#include <fmt/core.h>

using namespace drogon;

void Logger::initAndStart(const Json::Value &config) {
  app().registerPreSendingAdvice([](const drogon::HttpRequestPtr &req,
                                    const drogon::HttpResponsePtr &resp) {
    std::string methods[] = {"Get",    "Post",    "Head",  "Put",
                             "Delete", "Options", "Patch", "Invalid"};

    LOG_COMPACT_INFO << fmt::format("{} {} -> {} {}", methods[req->getMethod()],
                                    req->getPeerAddr().toIp(), req->getPath(),
                                    std::to_string(resp->getStatusCode()));
  });

  LOG_COMPACT_DEBUG << "Logger initialized and started";
}

void Logger::shutdown() { LOG_COMPACT_DEBUG << "Logger shutdown"; }
