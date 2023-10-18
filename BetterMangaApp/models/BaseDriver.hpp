#pragma once

#include "../../utils/Utils.hpp"
#include "Common.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ProxySettings {
  map<string, vector<string>> genre;
  map<string, string> headers;

  json toJson() {
    json result;

    result["genre"] = genre;
    result["headers"] = headers;

    return result;
  }
};

class BaseDriver {

public:
  string id;
  int recommendedChunkSize = 0;
  bool supportSuggestion;
  vector<Category> supportedCategories;
  ProxySettings proxySettings;

  virtual vector<Manga *> getManga(vector<string> ids, bool showDetails) = 0;

  virtual vector<string> getChapter(string id, string extraData) = 0;

  virtual vector<Manga *> getList(Category category, int page,
                                  Status status) = 0;

  virtual vector<string> getSuggestion(string keyword) {
    return vector<string>();
  };

  virtual vector<Manga *> search(string keyword, int page) = 0;

  virtual bool checkOnline() = 0;

  virtual string useProxy(const string &destination, const string &genre) {
    const char *proxyAddress = std::getenv("PROXY_ADDRESS");

    if (proxyAddress == nullptr) {
      return destination;
    }

    cpr::Parameters parameters = cpr::Parameters{
        {"driver", id},
        {"genre", genre},
        {"destination", destination},
    };

    cpr::Session session;
    session.SetUrl(proxyAddress);
    session.SetParameters(parameters);

    return session.GetFullRequestUrl();
  }
};
