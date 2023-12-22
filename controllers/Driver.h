#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class Driver : public drogon::HttpController<Driver> {
public:
  METHOD_LIST_BEGIN
  // use METHOD_ADD to add your custom processing function here;
  // METHOD_ADD(Driver::get, "/{2}/{1}", Get); // path is /Driver/{arg2}/{arg1}
  // METHOD_ADD(Driver::your_method_name, "/{1}/{2}/list", Get); // path is
  // /Driver/{arg1}/{arg2}/list ADD_METHOD_TO(Driver::your_method_name,
  // "/absolute/path/{1}/{2}/list", Get); // path is
  // /absolute/path/{arg1}/{arg2}/list

  METHOD_ADD(Driver::getDriver, "", Get, Options);
  METHOD_ADD(Driver::getProxy, "/proxy", Get, Options);
  METHOD_ADD(Driver::getOnline, "/online", Get, Options);

  METHOD_LIST_END

  void getDriver(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);

  void getProxy(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

  void getOnline(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);

  // your declaration of processing function maybe like this:
  // void get(const HttpRequestPtr& req, std::function<void (const
  // HttpResponsePtr &)> &&callback, int p1, std::string p2); void
  // your_method_name(const HttpRequestPtr& req, std::function<void (const
  // HttpResponsePtr &)> &&callback, double p1, int p2) const;
};
