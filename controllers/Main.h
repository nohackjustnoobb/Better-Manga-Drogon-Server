#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class Main : public drogon::HttpController<Main> {
public:
  METHOD_LIST_BEGIN
  // use METHOD_ADD to add your custom processing function here;
  // METHOD_ADD(Main::get, "/{2}/{1}", Get); // path is /Main/{arg2}/{arg1}
  // METHOD_ADD(Main::your_method_name, "/{1}/{2}/list", Get); // path is
  // /Main/{arg1}/{arg2}/list ADD_METHOD_TO(Main::your_method_name,
  // "/absolute/path/{1}/{2}/list", Get); // path is
  // /absolute/path/{arg1}/{arg2}/list

  ADD_METHOD_TO(Main::getInfo, "/", Get);
  ADD_METHOD_TO(Main::getList, "/list", Get);
  ADD_METHOD_TO(Main::getManga, "/manga", Get);
  ADD_METHOD_TO(Main::getManga, "/manga", Post);
  ADD_METHOD_TO(Main::getSuggestion, "/suggestion", Get);
  ADD_METHOD_TO(Main::getSearch, "/search", Get);
  ADD_METHOD_TO(Main::getChapter, "/chapter", Get);

  METHOD_LIST_END
  // your declaration of processing function maybe like this:
  // void get(const HttpRequestPtr& req, std::function<void (const
  // HttpResponsePtr &)> &&callback, int p1, std::string p2); void
  // your_method_name(const HttpRequestPtr& req, std::function<void (const
  // HttpResponsePtr &)> &&callback, double p1, int p2) const;

  void getInfo(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback);

  void getList(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback);

  void getManga(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);

  void getChapter(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);

  void getSuggestion(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);

  void getSearch(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);
};
