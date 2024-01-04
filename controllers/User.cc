#include "User.h"
#include "../utils/Utils.hpp"

#include <drogon/orm/Result.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <time.h>
#include <utility>

using json = nlohmann::json;

// Add definition of your processing function here

void User::create(const HttpRequestPtr &req,
                  std::function<void(const HttpResponsePtr &)> &&callback) {
  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    json body = json::parse(req->getBody());
    string email = body["email"].get<string>();
    string password = body["password"].get<string>();
    string key = body["key"].get<string>();

    for (char &c : email)
      c = std::tolower(c);

    try {
      const char *registerKey = std::getenv("REGISTER_KEY");
      if (registerKey != nullptr && registerKey != key)
        throw "Register Key is not matching.";

      if (!verifyEmail(email))
        throw "The email is not valid.";

      if (!verifyPassword(password))
        throw "Passwords must contain at least eight characters, one letter "
              "and one number.";

      try {
        auto user = userModel();
        user.setEmail(email);
        user.setPassword(hashPassword(password));
        userMapper.insert(user);

        json info;
        info["email"] = user.getValueOfEmail();
        resp->setStatusCode(k200OK);
        resp->setBody(info.dump());
      } catch (...) {
        throw "An unexpected error occurred when trying to create user.";
      }
    } catch (const char *e) {
      json error;
      error["error"] = e;
      resp->setStatusCode(k400BadRequest);
      resp->setBody(error.dump());
    }

  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\":\"\\\"email\\\", \\\"password\\\" or "
                  "\\\"key\\\" are missing.\"}");
  }

  callback(resp);
}

void User::token(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback) {
  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    json body = json::parse(req->getBody());
    string email = body["email"].get<string>();
    string password = body["password"].get<string>();

    try {
      auto user = userMapper.findOne(
          Criteria(userModel::Cols::_EMAIL, CompareOperator::EQ, email));
      if (!checkPassword(user.getValueOfPassword(), password))
        throw "";

      json result;
      result["token"] = createToken(&user);

      resp->setStatusCode(k200OK);
      resp->setBody(result.dump());

    } catch (...) {
      resp->setStatusCode(k400BadRequest);
      resp->setBody(
          "{\"error\":\"\\\"email\\\" or \\\"password\\\" are incorrect.\"}");
    }

  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody(
        "{\"error\":\"\\\"email\\\" or \\\"password\\\" are missing.\"}");
  }

  callback(resp);
}

void User::me(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) {
  auto user = tryAuthenticate(req, std::move(callback));

  if (user.getId() == nullptr)
    return;

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setStatusCode(k200OK);

  json body;
  if (req->getMethod() == Post) {
    try {
      json reqBody = json::parse(req->getBody());
      string oldPassword = reqBody["oldPassword"].get<string>();
      string newPassword = reqBody["newPassword"].get<string>();

      if (!checkPassword(user.getValueOfPassword(), oldPassword))
        throw "The old password is incorrect.";

      if (!verifyPassword(newPassword))
        throw "Passwords must contain at least eight characters, one letter "
              "and one number.";

      user.setPassword(hashPassword(newPassword));
      userMapper.update(user);
    } catch (const char *e) {
      body["error"] = e;
      resp->setStatusCode(k400BadRequest);
    } catch (...) {
      body["error"] = "\"oldPassword\" or \"newPassword\" are missing.";
      resp->setStatusCode(k400BadRequest);
    }
  }

  if (resp->statusCode() == k200OK)
    body["email"] = user.getValueOfEmail();

  resp->setBody(body.dump());

  callback(resp);
}

void User::clear(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback) {
  auto user = tryAuthenticate(req, std::move(callback));

  if (user.getId() == nullptr)
    return;

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setStatusCode(k204NoContent);

  json body;
  try {
    string password = json::parse(req->getBody())["password"].get<string>();

    if (!checkPassword(user.getValueOfPassword(), password))
      throw "Incorrect password.";

    collectionMapper.deleteBy(Criteria(
        historyModel::Cols::_USERID, CompareOperator::EQ, user.getValueOfId()));
    historyMapper.deleteBy(Criteria(historyModel::Cols::_USERID,
                                    CompareOperator::EQ, user.getValueOfId()));

  } catch (const char *e) {
    body["error"] = e;
    resp->setStatusCode(k400BadRequest);
  } catch (...) {
    body["error"] = "\"password\" is missing.";
    resp->setStatusCode(k400BadRequest);
  }

  if (resp->statusCode() == k400BadRequest) {
    resp->setBody(body.dump());
  }

  callback(resp);
}

void User::collections(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  auto user = tryAuthenticate(req, std::move(callback));

  if (user.getId() == nullptr)
    return;

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    if (req->getMethod() == Get) {
      json body = json::array();
      for (const auto &item : collectionMapper.findBy(
               Criteria(mangaModel::Cols::_USERID, CompareOperator::EQ,
                        user.getValueOfId()))) {
        json manga;
        manga["id"] = item.getValueOfId();
        manga["driver"] = item.getValueOfDriver();
        body.push_back(manga);
      }

      resp->setStatusCode(k200OK);
      resp->setBody(body.dump());
    } else if (req->getMethod() == Post) {
      json body = json::parse(req->getBody());
      bool isSuccess = false;
      for (auto item : body) {
        try {
          string id = item["id"].get<string>();
          string driver = item["driver"].get<string>();

          try {
            collectionMapper.findByPrimaryKey(
                {user.getValueOfId(), id, driver});
          } catch (...) {
            mangaModel manga;
            manga.setDriver(driver);
            manga.setId(id);
            manga.setUserid(user.getValueOfId());
            collectionMapper.insert(manga);
            isSuccess = true;
          }
        } catch (...) {
        }
      }

      if (!isSuccess)
        throw "";

      resp->setStatusCode(k201Created);
      resp->setBody("{\"success\":\"Updated Collections\"}");
    } else if (req->getMethod() == Delete) {
      auto parameters = req->getParameters();
      auto tryDriver = parameters.find("driver");
      auto tryId = parameters.find("id");

      if (tryDriver == parameters.end() || tryId == parameters.end()) {
        resp->setStatusCode(k400BadRequest);
        resp->setBody(
            "{\"error\":\"\\\"driver\\\" or \\\"ids\\\" are missing.\"}");
      } else {
        size_t result = collectionMapper.deleteByPrimaryKey(
            {user.getValueOfId(), tryId->second, tryDriver->second});

        if (result) {
          resp->setStatusCode(k204NoContent);
          resp->setBody("{\"success\":\"Updated Collections\"}");
        } else {
          resp->setStatusCode(k404NotFound);
          resp->setBody(
              "{\"error\":\"The corresponding manga was not found.\"}");
        }
      }
    }
  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\":\"An unexpected error occurred when syncing "
                  "the collections.\"}");
  }

  callback(resp);
}

void User::histories(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback) {
  auto user = tryAuthenticate(req, std::move(callback));

  if (user.getId() == nullptr)
    return;

  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    if (req->getMethod() == Post) {
      try {
        json body = json::parse(req->getBody());

        for (auto item : body) {
          try {
            historyModel record;
            record.setUserid(user.getValueOfId());
            record.setId(item["id"].get<string>());
            record.setDriver(item["driver"].get<string>());
            record.setTitle(item["title"].get<string>());
            record.setThumbnail(item["thumbnail"].get<string>());
            record.setLatest(item["latest"].get<string>());
            record.setNew(item["new"].get<bool>() ? "1" : "0");
            record.setDatetime(item["datetime"].get<u_int64_t>());
            record.setUpdatedatetime(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());

            if (item["chapterId"].is_null())
              record.setChapteridToNull();
            else
              record.setChapterid(item["chapterId"].get<string>());

            if (item["chapterTitle"].is_null())
              record.setChaptertitleToNull();
            else
              record.setChaptertitle(item["chapterTitle"].get<string>());

            if (item["page"].is_null())
              record.setPageToNull();
            else
              record.setPage(item["page"].get<int>());

            try {
              auto currentRecord = historyMapper.findByPrimaryKey(
                  {user.getValueOfId(), record.getValueOfId(),
                   record.getValueOfDriver()});

              if (currentRecord.getValueOfDatetime() >=
                  record.getValueOfDatetime())
                continue;

              historyMapper.update(record);
            } catch (...) {
              historyMapper.insert(record);
            }
          } catch (...) {
          }
        }
      } catch (...) {
      }
    }

    auto parameters = req->getParameters();
    auto tryDatetime = parameters.find("datetime");
    auto tryPage = parameters.find("page");

    int page = 1;
    if (tryPage != parameters.end()) {
      int parsedPage = std::stoi(tryPage->second);
      if (parsedPage > 0)
        page = parsedPage;
    }

    uint64_t datetime = -1;
    if (tryDatetime != parameters.end())
      datetime = std::stoull(tryDatetime->second);

    auto mapper = historyMapper.orderBy(historyModel::Cols::_UPDATEDATETIME)
                      .limit(50)
                      .offset((page - 1) * 50);

    Criteria criteria =
        datetime == -1 ? Criteria(historyModel::Cols::_USERID,
                                  CompareOperator::EQ, user.getValueOfId())
                       : (Criteria(historyModel::Cols::_USERID,
                                   CompareOperator::EQ, user.getValueOfId()) &&
                          Criteria(historyModel::Cols::_UPDATEDATETIME,
                                   CompareOperator::GE, datetime));

    vector<historyModel> result = mapper.findBy(criteria);
    size_t count = mapper.count(criteria);
    resp->addHeader("Is-Next", count > page * 50 ? "1" : "0");

    json body = json::array();
    for (const auto &item : result) {
      json record;
      record["driver"] = item.getValueOfDriver();
      record["id"] = item.getValueOfId();
      record["title"] = item.getValueOfTitle();
      record["thumbnail"] = item.getValueOfThumbnail();
      record["latest"] = item.getValueOfLatest();
      record["datetime"] = item.getValueOfDatetime();
      record["new"] = item.getValueOfNew() == "1";
      if (item.getPage())
        record["page"] = item.getValueOfPage();
      else
        record["page"] = nullptr;

      if (item.getChapterid())
        record["chapterId"] = item.getValueOfChapterid();
      else
        record["chapterId"] = nullptr;

      if (item.getChaptertitle())
        record["chapterTitle"] = item.getValueOfChaptertitle();
      else
        record["chapterTitle"] = nullptr;

      body.push_back(record);
    }

    resp->setBody(body.dump());
    resp->setStatusCode(req->getMethod() == Get ? k200OK : k202Accepted);
  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\":\"An unexpected error occurred when syncing "
                  "the histories.\"}");
  }

  callback(resp);
}

bool User::verifyEmail(const string &email) {
  re2::StringPiece input(email);
  return RE2::FullMatch(
      input,
      R"((?:[a-z0-9!#$%&'*+\=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+\=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))");
};

bool User::verifyPassword(const string &password) {
  return std::regex_match(
      password, std::regex(R"(^(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d]{8,}$)"));
}

string User::hashPassword(const string &password) {
  const size_t saltLen = 16;
  uint8_t salt[saltLen];

  RAND_bytes(salt, saltLen);

  const char *cPassword = password.c_str();
  char encoded[128];

  argon2id_hash_encoded(2, 1 << 16, 4, cPassword, sizeof(cPassword), salt,
                        sizeof(salt), 32, encoded, sizeof(encoded));

  return string(encoded);
}

bool User::checkPassword(const string &hashedPassword, const string &password) {
  const char *c_password = password.c_str();
  return argon2id_verify(hashedPassword.c_str(), c_password,
                         sizeof(c_password)) == 0;
}

string User::createToken(userModel *user) {
  if (secretKey == nullptr)
    throw "Failed to get SECRET_KEY in the environment.";

  auto token = jwt::create<jwt::traits::nlohmann_json>()
                   .set_issuer("BetterMangaApp")
                   .set_issued_at(std::chrono::system_clock::now())
                   .set_payload_claim("uid", user->getValueOfId())
                   .set_key_id(sha256(user->getValueOfPassword()))
                   .set_type("JWT")
                   .sign(jwt::algorithm::hs256{secretKey});

  return token;
}

userModel
User::tryAuthenticate(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback) {
  HttpResponsePtr resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);

  try {
    string authorization = req->getHeader("Authorization");

    if (authorization.empty())
      throw "No token is provided.";

    if (secretKey == nullptr)
      throw "Failed to get SECRET_KEY in the environment.";

    try {
      string token = split(authorization, " ").at(1);

      auto decoded = jwt::decode<jwt::traits::nlohmann_json>(token);
      verifier.verify(decoded);

      int uid = decoded.get_payload_claim("uid").as_int();
      auto user = userMapper.findByPrimaryKey(uid);
      string keyId = decoded.get_key_id();

      if (keyId != sha256(user.getValueOfPassword()))
        throw "";

      return user;
    } catch (...) {
      throw "The token is not valid.";
    }

  } catch (const char *error) {
    json body;
    body["error"] = error;

    resp->setStatusCode(k401Unauthorized);
    resp->setBody(body.dump());
  } catch (...) {
    resp->setStatusCode(k400BadRequest);
    resp->setBody("{\"error\":\"An unexpected error occurred when trying to "
                  "authenticate the user.\"}");
  }

  callback(resp);
  return userModel();
}
