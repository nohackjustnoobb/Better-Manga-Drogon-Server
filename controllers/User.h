#pragma once

#include "../models/History.h"
#include "../models/Manga.h"
#include "../models/User.h"
#include "../utils/TypeAliases.hpp"

#include <drogon/HttpController.h>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>

using namespace drogon;
using namespace drogon::orm;

using userModel = drogon_model::sqlite3::User;
using mangaModel = drogon_model::sqlite3::Manga;
using historyModel = drogon_model::sqlite3::History;

class User : public drogon::HttpController<User> {
public:
  METHOD_LIST_BEGIN
  // use METHOD_ADD to add your custom processing function here;
  // METHOD_ADD(User::get, "/{2}/{1}", Get); // path is /User/{arg2}/{arg1}
  // METHOD_ADD(User::your_method_name, "/{1}/{2}/list", Get); // path is
  // /User/{arg1}/{arg2}/list ADD_METHOD_TO(User::your_method_name,
  // "/absolute/path/{1}/{2}/list", Get); // path is
  // /absolute/path/{arg1}/{arg2}/list

  METHOD_ADD(User::create, "/create", Post, Options);
  METHOD_ADD(User::token, "/token", Post, Options);
  METHOD_ADD(User::clear, "/clear", Post, Options);

  METHOD_ADD(User::me, "/me", Get, Post, Options);
  METHOD_ADD(User::collections, "/collections", Get, Post, Delete, Options);
  METHOD_ADD(User::histories, "/histories", Get, Post, Options);

  METHOD_LIST_END
  // your declaration of processing function maybe like this:
  // void get(const HttpRequestPtr& req, std::function<void (const
  // HttpResponsePtr &)> &&callback, int p1, std::string p2); void
  // your_method_name(const HttpRequestPtr& req, std::function<void (const
  // HttpResponsePtr &)> &&callback, double p1, int p2) const;

  void create(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback);

  void token(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback);

  void me(const HttpRequestPtr &req,
          std::function<void(const HttpResponsePtr &)> &&callback);

  void clear(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback);

  void collections(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);

  void histories(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);

private:
  Mapper<userModel> userMapper = Mapper<userModel>(app().getDbClient());
  Mapper<mangaModel> collectionMapper = Mapper<mangaModel>(app().getDbClient());
  Mapper<historyModel> historyMapper =
      Mapper<historyModel>(app().getDbClient());

  const char *secretKey = std::getenv("SECRET_KEY");
  jwt::verifier<jwt::default_clock, jwt::traits::nlohmann_json> verifier =
      jwt::verify<jwt::traits::nlohmann_json>()
          .allow_algorithm(jwt::algorithm::hs256{secretKey})
          .with_issuer("BetterMangaApp");

  bool verifyEmail(const string &email);
  bool verifyPassword(const string &password);

  string hashPassword(const string &password);
  bool checkPassword(const string &password, const string &passwordToCheck);

  string createToken(userModel *user);

  userModel
  tryAuthenticate(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback);
};
