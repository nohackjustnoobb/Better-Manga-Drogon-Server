#include "../../utils/Converter.hpp"
#include "../../utils/Utils.hpp"
#include "../models/BaseDriver.hpp"
#include "../models/Manga.hpp"

#include <algorithm>
#include <cpr/cpr.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <thread>

using json = nlohmann::json;

class MHR : public BaseDriver {
public:
  MHR() {
    id = "MHR";
    supportSuggestion = true;
    for (const auto &pair : categoryId) {
      supportedCategories.push_back(pair.first);
    }
    proxySettings = {{{"thumbnail",
                       {
                           "https://mhfm1us.cdnmanhua.net",
                           "https://mhfm2us.cdnmanhua.net",
                           "https://mhfm3us.cdnmanhua.net",
                           "https://mhfm4us.cdnmanhua.net",
                           "https://mhfm5us.cdnmanhua.net",
                           "https://mhfm6us.cdnmanhua.net",
                           "https://mhfm7us.cdnmanhua.net",
                           "https://mhfm8us.cdnmanhua.net",
                           "https://mhfm9us.cdnmanhua.net",
                           "https://mhfm10us.cdnmanhua.net",
                       }},
                      {"manga", {}}},
                     {{{"referer", "http://www.dm5.com/dm5api/"}}}};
  }

  vector<Manga *> getManga(vector<string> ids, bool showDetails) override {
    vector<Manga *> result;

    if (showDetails) {
      vector<std::thread> threads;
      std::mutex mutex;
      auto fetchId = [&](const string &id, vector<Manga *> &result) {
        map<string, string> query = {
            {"mangaId", id},
            {"mangaDetailVersion", ""},
        };

        query.insert(baseQuery.begin(), baseQuery.end());
        query["gsn"] = hash(query);

        cpr::Response r = cpr::Get(cpr::Url{baseUrl + "v1/manga/getDetail"},
                                   mapToParameters(query), header);
        json data = json::parse(r.text);

        Manga *manga = convertDetails(data["response"]);
        std::lock_guard<std::mutex> guard(mutex);
        result.push_back(manga);
      };

      // create threads
      for (const string &id : ids) {
        threads.push_back(std::thread(fetchId, std::ref(id), std::ref(result)));
      }

      // wait for threads to finish
      for (auto &th : threads) {
        th.join();
      }

    } else {

      map<string, string> query;
      query.insert(baseQuery.begin(), baseQuery.end());

      // construct body
      json body;
      body["mangaCoverimageType"] = 1;
      body["bookIds"] = json::array();
      body["somanIds"] = json::array();
      body["mangaIds"] = ids;
      string dumpBody = body.dump();

      query["gsn"] = hash(query, dumpBody);

      // add content-type to headers temporarily
      header["Content-Type"] = "application/json";
      cpr::Response r =
          cpr::Post(cpr::Url{baseUrl + "v2/manga/getBatchDetail"},
                    mapToParameters(query), header, cpr::Body{dumpBody});

      // remove content-type from headers
      header.erase("Content-Type");

      // parse the data
      json data = json::parse(r.text);

      for (const json &manga : data["response"]["mangas"]) {
        result.push_back(convert(manga));
      }
    }

    return result;
  };

  vector<string> getChapter(string id, string extraData) override {
    map<string, string> query = {
        {"mangaSectionId", id}, {"mangaId", extraData}, {"netType", "1"},
        {"loadreal", "1"},      {"imageQuality", "2"},

    };

    query.insert(baseQuery.begin(), baseQuery.end());
    query["gsn"] = hash(query);

    cpr::Response r = cpr::Get(cpr::Url{baseUrl + "v1/manga/getRead"},
                               mapToParameters(query), header);
    json data = json::parse(r.text)["response"];

    string baseUrl = data["hostList"].get<vector<string>>().at(0);
    vector<string> result;
    string urlQuery = data["query"].get<string>();

    for (const json &path : data["mangaSectionImages"]) {
      string url = baseUrl + path.get<string>() + urlQuery;

      result.push_back(url);
    }

    return result;
  };

  vector<Manga *> getList(Category category, int page, Status status) override {

    map<string, string> query = {
        {"subCategoryType", "0"},
        {"subCategoryId", std::to_string(categoryId[category])},
        {"start", std::to_string((page - 1) * 50)},
        {"status", std::to_string(status)},
        {"limit", "50"},
        {"sort", "0"},
    };

    query.insert(baseQuery.begin(), baseQuery.end());
    query["gsn"] = hash(query);

    cpr::Response r = cpr::Get(cpr::Url{baseUrl + "v2/manga/getCategoryMangas"},
                               mapToParameters(query), header);
    json data = json::parse(r.text);

    vector<Manga *> result;
    for (const json &manga : data["response"]["mangas"]) {
      result.push_back(convert(manga));
    }

    return result;
  };

  vector<string> getSuggestion(string keyword) override {
    keyword.erase(std::remove(keyword.begin(), keyword.end(), '/'),
                  keyword.end());

    map<string, string> query = {
        {"keywords", chineseConverter.toSimplified(keyword)},
        {"mh_is_anonymous", "0"},
    };

    query.insert(baseQuery.begin(), baseQuery.end());
    query["gsn"] = hash(query);

    cpr::Response r = cpr::Get(cpr::Url{baseUrl + "v1/search/getKeywordMatch"},
                               mapToParameters(query), header);

    // parse & extract the data
    json data = json::parse(r.text);
    vector<string> result;

    for (const json &item : data["response"]["items"]) {
      result.push_back(item["mangaName"].get<string>());
    }

    return result;
  };

  vector<Manga *> search(string keyword, int page) override {
    keyword.erase(std::remove(keyword.begin(), keyword.end(), '/'),
                  keyword.end());

    map<string, string> query = {
        {"keywords", chineseConverter.toSimplified(keyword)},
        {"start", std::to_string((page - 1) * 50)},
        {"limit", "50"},
    };

    query.insert(baseQuery.begin(), baseQuery.end());
    query["gsn"] = hash(query);

    cpr::Response r = cpr::Get(cpr::Url{baseUrl + "v1/search/getSearchManga"},
                               mapToParameters(query), header);
    json data = json::parse(r.text);

    vector<string> result;
    for (const json &manga : data["response"]["result"]) {
      result.push_back(std::to_string(manga["mangaId"].get<int>()));
    }

    return getManga(result, false);
  };

  bool checkOnline() override {
    return cpr::Get(cpr::Url{baseUrl}, cpr::Timeout{5000}).status_code != 0;
  };

private:
// disable warning for deprecated functions
// which don't have standardized replacements
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wstringConverter;
#pragma GCC diagnostic pop

  string hashKey = "4e0a48e1c0b54041bce9c8f0e036124d";
  string baseUrl = "https://hkmangaapi.manhuaren.com/";
  Converter chineseConverter;
  map<string, string> baseQuery = {
      {"gak", "android_manhuaren2"},
      {"gaui", "462099841"},
      {"gft", "json"},
      {"gui", "462099841"},
  };
  cpr::Header header = cpr::Header{
      {"Authorization",
       "YINGQISTS2 "
       "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9."
       "eyJpc19mcm9tX3JndCI6ZmFsc2UsInVzZXJfbGVnYWN5X2lkIjo0NjIwOTk4NDEsImRldml"
       "jZV9pZCI6Ii0zNCw2OSw2MSw4MSw2LDExNCw2MSwtMzUsLTEsNDgsNiwzNSwtMTA3LC0xMj"
       "IsLTExLC04NywxMjcsNjQsLTM4LC03LDUwLDEzLC05NCwtMTcsLTI3LDkyLC0xNSwtMTIwL"
       "C0zNyw3NCwtNzksNzgiLCJ1dWlkIjoiOTlmYTYzYjQtNjFmNy00ODUyLThiNDMtMjJlNGY3"
       "YzY2MzhkIiwiY3JlYXRldGltZV91dGMiOiIyMDIzLTA3LTAzIDAyOjA1OjMwIiwibmJmIjo"
       "xNjg4MzkzMTMwLCJleHAiOjE2ODgzOTY3MzAsImlhdCI6MTY4ODM5MzEzMH0."
       "IJAkDs7l3rEvURHiy06Y2STyuiIu-CYUk5E8en4LU0_mrJ83hKZR1nVqKiAY9ry_"
       "6ZmFzVfg-ap_TXTF6GTqihyM-nmEpD2NVWeWZ5VHWVgJif4ezB4YTs0YEpnVzYCk_"
       "x4p0wU2GYbqf1BFrNO7PQPMMPDGfaCTUqI_Pe2B0ikXMaN6CDkMho26KVT3DK-"
       "xytc6lO92RHvg65Hp3xC1qaonQXdws13wM6WckUmrswItroy9z38hK3w0rQgXOK2mu3o_"
       "4zOKLGfq5JpqOCNQCLJgQ0_jFXhMtaz6E_fMZx54fZHfF1YrA-"
       "tfs7KFgiYxMb8PnNILoniFrQhvET3y-Q"},
      {"X-Yq-Yqci",
       "{\"av\":\"1.3.8\",\"cy\":\"HK\",\"lut\":\"1662458886867\",\"nettype\":"
       "1,\"os\":2,\"di\":\"733A83F2FD3B554C3C4E4D46A307D560A52861C7\",\"fcl\":"
       "\"appstore\",\"fult\":\"1662458886867\",\"cl\":\"appstore\",\"pi\":"
       "\"\",\"token\":\"\",\"fut\":\"1662458886867\",\"le\":\"en-HK\",\"ps\":"
       "\"1\",\"ov\":\"16.4\",\"at\":2,\"rn\":\"1668x2388\",\"ln\":\"\",\"pt\":"
       "\"com.CaricatureManGroup.CaricatureManGroup\",\"dm\":\"iPad8,6\"}"},
      {"User-Agent",
       "Mozilla/5.0 (Linux; Android 12; sdk_gphone64_arm64 "
       "Build/SE1A.220630.001; wv) AppleWebKit/537.36 (KHTML, like Gecko) "
       "Version/4.0 Chrome/91.0.4472.114 Mobile Safari/537.36"}};
  map<Category, int> categoryId = {
      {All, 0},     {Passionate, 31}, {Love, 26},       {Campus, 1},
      {Yuri, 3},    {BL, 27},         {Adventure, 2},   {Harem, 8},
      {SciFi, 25},  {War, 12},        {Suspense, 17},   {Speculation, 33},
      {Funny, 37},  {Fantasy, 14},    {Magic, 15},      {Horror, 29},
      {Ghosts, 20}, {History, 4},     {FanFi, 30},      {Sports, 34},
      {Hentai, 36}, {Mecha, 40},      {Restricted, 61}, {Otokonoko, 5}};
  map<string, Category> categoryText = {
      {"热血", Passionate}, {"恋爱", Love},         {"爱情", Love},
      {"校园", Campus},     {"百合", Yuri},         {"彩虹", BL},
      {"冒险", Adventure},  {"后宫", Harem},        {"科幻", SciFi},
      {"战争", War},        {"悬疑", Suspense},     {"推理", Speculation},
      {"搞笑", Funny},      {"奇幻", Fantasy},      {"魔法", Magic},
      {"恐怖", Horror},     {"神鬼", Ghosts},       {"历史", History},
      {"同人", FanFi},      {"运动", Sports},       {"绅士", Hentai},
      {"机甲", Mecha},      {"限制级", Restricted}, {"伪娘", Otokonoko}};

  // create a MD5 hash from a list of strings
  string hash(vector<string> &list) {
    string result;

    for (const string &str : list)
      result += str;

    result = urlEncode(result);

    return md5(result);
  }

  string hash(map<string, string> query) {
    vector<string> flatQuery;

    flatQuery.push_back(hashKey);
    flatQuery.push_back("GET");

    for (const auto &pair : query) {
      flatQuery.push_back(pair.first);
      flatQuery.push_back(pair.second);
    }

    flatQuery.push_back(hashKey);

    return hash(flatQuery);
  }

  string hash(map<string, string> query, string body) {
    vector<string> flatQuery;

    flatQuery.push_back(hashKey);
    flatQuery.push_back("POST");

    flatQuery.push_back("body");
    flatQuery.push_back(body);

    for (const auto &pair : query) {
      flatQuery.push_back(pair.first);
      flatQuery.push_back(pair.second);
    }

    flatQuery.push_back(hashKey);

    return hash(flatQuery);
  }

  Manga *convert(const json &data) {
    return new Manga(this, std::to_string(data["mangaId"].get<int>()),
                     data["mangaName"].get<string>(),
                     extractThumbnail(data["mangaCoverimageUrl"].get<string>()),
                     data.contains("mangaNewestContent")
                         ? data["mangaNewestContent"].get<string>()
                         : data["mangaNewsectionName"].get<string>(),
                     data["mangaIsOver"].get<int>() == 1);
  }

  Manga *convertDetails(const json &data) {
    vector<Chapter> serial;
    vector<Chapter> extra;
    string id = std::to_string(data["mangaId"].get<int>());

    auto pushChapters = [](const json &chaptersData,
                           vector<Chapter> &chapters) {
      for (const json &chapter : chaptersData) {
        chapters.push_back({chapter["sectionName"].get<string>(),
                            std::to_string(chapter["sectionId"].get<int>())});
      }
    };

    pushChapters(data["mangaWords"], serial);
    pushChapters(data["mangaEpisode"], extra);
    pushChapters(data["mangaRolls"], extra);

    vector<Category> categories;
    string rawCategoriesText = data["mangaTheme"].get<string>();

    for (const auto &pair : categoryText) {
      if (rawCategoriesText.find(pair.first) != string::npos) {
        categories.push_back(pair.second);
      }
    }

    return new DetailsManga(
        this, id, data["mangaName"].get<string>(),
        extractThumbnail(data["mangaPicimageUrl"].get<string>()),
        data["mangaNewsectionName"].get<string>(),
        data["mangaAuthors"].get<vector<string>>(),
        data["mangaIsOver"].get<int>() == 1, data["mangaIntro"].get<string>(),
        categories, {serial, extra, id});
  }

  string extractThumbnail(const string &url) {
    // make a copy
    string thumbnail = url;
    // swap the source to a faster alternative
    RE2::GlobalReplace(&thumbnail, RE2("cdndm5.com"), "cdnmanhua.net");

    return thumbnail;
  }

  vector<string> extractAuthors(const string authorsString) {
    vector<string> authors = split(authorsString, RE2("，|,|、| |/"));

    authors.erase(std::remove_if(authors.begin(), authors.end(),
                                 [](string str) {
                                   return str.find_first_not_of(" \t\n\r") ==
                                          string::npos;
                                 }),
                  authors.end());

    return authors;
  }
};