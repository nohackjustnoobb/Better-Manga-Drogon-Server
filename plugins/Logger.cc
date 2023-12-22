/**
 *
 *  Logger.cc
 *
 */

#include "Logger.h"
#include <drogon/HttpController.h>

using namespace drogon;

void Logger::initAndStart(const Json::Value &config) {
  app().registerPreSendingAdvice([](const drogon::HttpRequestPtr &req,
                                    const drogon::HttpResponsePtr &resp) {
    std::string methods[] = {"Get",    "Post",    "Head",  "Put",
                             "Delete", "Options", "Patch", "Invalid"};

    std::string msg;
    msg += methods[req->getMethod()];
    msg += " ";
    msg += req->getPeerAddr().toIp();
    msg += " -> ";
    msg += req->getPath();
    msg += " ";
    msg += std::to_string(resp->getStatusCode());

    LOG_COMPACT_INFO << msg;
  });

  LOG_COMPACT_DEBUG << "Logger initialized and started";
}

void Logger::shutdown() { LOG_COMPACT_DEBUG << "Logger shutdown"; }
