#include "../../utils/Node.hpp"
#include "../../utils/Utils.hpp"
#include "../../utils/lz-string.hpp"
#include "../models/ActiveDriver.hpp"
#include "../models/Manga.hpp"

#include <codecvt>
#include <cpr/cpr.h>
#include <ctime>
#include <locale>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

class MHG : public ActiveDriver {
public:
  MHG() {
    id = "MHG";
    supportSuggestion = false;
    recommendedChunkSize = 5;
    version = "0.1.0-beta.0";
    timeout = 10;

    for (const auto &pair : categoryText) {
      supportedCategories.push_back(pair.first);
    }
    proxySettings = {{{"thumbnail", {}}, {"manga", {}}},
                     {{"referer", "https://tw.manhuagui.com"}}};
  }

  vector<Manga *> getManga(vector<string> ids, bool showDetails,
                           string proxy) override {
    vector<Manga *> result;

    bool isError = false;
    vector<std::thread> threads;
    std::mutex mutex;
    auto fetchId = [&](const string &id, vector<Manga *> &result) {
      try {
        cpr::Url url{baseUrl + "comic/" + id};
        cpr::Response r = proxy == ""
                              ? cpr::Get(url, cpr::Timeout{5000})
                              : cpr::Get(url, cpr::Timeout{5000},
                                         cpr::Proxies{{"https", proxy}});

        Node *body = new Node(r.text);

        Manga *manga = extractDetails(body, id, showDetails);
        std::lock_guard<std::mutex> guard(mutex);
        result.push_back(manga);
      } catch (...) {
        isError = true;
      }
    };

    // create threads
    for (const string &id : ids) {
      threads.push_back(std::thread(fetchId, std::ref(id), std::ref(result)));
    }

    // wait for threads to finish
    for (auto &th : threads) {
      th.join();
    }

    if (isError)
      throw "Failed to fetch manga";

    return result;
  };

  vector<string> getChapter(string id, string extraData,
                            string proxy) override {
    vector<string> result;

    cpr::Url url{baseUrl + "comic/" + extraData + "/" + id + ".html"};
    cpr::Response r = proxy == "" ? cpr::Get(url, cpr::Timeout{5000})
                                  : cpr::Get(url, cpr::Timeout{5000},
                                             cpr::Proxies{{"https", proxy}});

    re2::StringPiece input(r.text);
    string encoded, valuesString, len1String, len2String;

    if (RE2::PartialMatch(
            input,
            RE2(R"((?m)^.*\}\(\'(.*)\',(\d*),(\d*),\'([\w|\+|\/|=]*)\'.*$)"),
            &encoded, &len1String, &len2String, &valuesString)) {
      int len1, len2;
      len1 = std::stoi(len1String);
      len2 = std::stoi(len2String);

      valuesString = convert.to_bytes(
          lzstring::decompressFromBase64(convert.from_bytes(valuesString)));

      result = decodeChapters(encoded, len1, len2, valuesString);
    } else {
      throw "Can't parse the data";
    }

    return result;
  };

  vector<Manga *> getList(Category category, int page, Status status) override {
    string url = baseUrl + "list/";
    string filter;

    if (category != All)
      filter += categoryText[category];

    if (status != Any) {
      if (category != All)
        filter += "_";
      filter += status == OnGoing ? "lianzai" : "wanjie";
    }

    if (!filter.empty())
      filter += "/";

    // add filter
    url += filter;
    // add page
    url += "index_p" + std::to_string(page) + ".html";

    cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Timeout{5000});
    Node *body = new Node(r.text);

    vector<Manga *> result;
    vector<Node *> items = body->findAll("#contList > li");

    for (Node *node : items) {
      result.push_back(extractManga(node));
    }

    delete body;
    releaseMemory(items);

    return result;
  };

  vector<Manga *> search(string keyword, int page) override {
    cpr::Response r = cpr::Get(cpr::Url{baseUrl + "s/" + keyword + "_p" +
                                        std::to_string(page) + ".html"},
                               cpr::Timeout{5000});
    Node *body = new Node(r.text);

    vector<Manga *> result;
    vector<Node *> items = body->findAll("div.book-result > ul > li.cf");

    for (Node *node : items) {
      result.push_back(extractManga(node));
    }

    delete body;
    releaseMemory(items);

    return result;
  };

  vector<PreviewManga> getUpdates(string proxy = "") override {
    cpr::Url url{"https://tw.manhuagui.com/update/d3.html"};
    cpr::Response r = proxy == "" ? cpr::Get(url, cpr::Timeout{5000})
                                  : cpr::Get(url, cpr::Timeout{5000},
                                             cpr::Proxies{{"https", proxy}});

    Node *body = new Node(r.text);

    vector<PreviewManga> result;
    vector<Node *> items =
        body->findAll("div.latest-cont > div.latest-list > ul > li");
    for (Node *node : items) {
      Node *details = node->find("a");
      string id = details->getAttribute("href");
      id = id = id.substr(7, id.length() - 8);

      Node *latestNode = details->find("span.tt");
      string latest = latestNode->text();
      RE2::GlobalReplace(&latest, RE2("更新至|共"), "");
      latest = strip(latest);

      delete latestNode;
      delete details;

      result.push_back({id, latest});
    }

    return result;
  }

  bool checkOnline() override {
    return cpr::Get(cpr::Url{baseUrl}, cpr::Timeout{5000}).status_code != 0;
  };

private:
// disable warning for deprecated functions
// which don't have standardized replacements
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> convert;
#pragma GCC diagnostic pop

  string baseUrl = "https://tw.manhuagui.com/";
  map<Category, string> categoryText = {{All, ""},
                                        {Passionate, "rexue"},
                                        {Love, "aiqing"},
                                        {Campus, "xiaoyuan"},
                                        {Yuri, "baihe"},
                                        {BL, "danmei"},
                                        {Adventure, "maoxian"},
                                        {Harem, "hougong"},
                                        {SciFi, "kehuan"},
                                        {War, "zhanzheng"},
                                        {Suspense, "xuanyi"},
                                        {Speculation, "tuili"},
                                        {Funny, "gaoxiao"},
                                        {Fantasy, "mohuan"},
                                        {Magic, "mofa"},
                                        {Horror, "kongbu"},
                                        {Ghosts, "shengui"},
                                        {History, "lishi"},
                                        {Sports, "jingji"},
                                        {Mecha, "jizhan"},
                                        {Otokonoko, "weiniang"}};

  Manga *extractDetails(Node *node, const string &id, const bool showDetails) {
    Node *details = node->find("div.book-cover");

    Node *thumbnailNode = details->find("img");
    string thumbnail = "https:" + thumbnailNode->getAttribute("src");
    string title = thumbnailNode->getAttribute("alt");
    delete thumbnailNode;

    Node *isEndedNode = details->tryFind("span.finish");
    bool isEnded = isEndedNode != nullptr;
    delete isEndedNode;

    Node *latestNode = details->find("span.text");
    string latest = latestNode->text();
    RE2::GlobalReplace(&latest, RE2("更新至："), "");
    latest = strip(latest);
    delete latestNode;

    delete details;
    if (!showDetails) {
      return new Manga(this, id, title, thumbnail, latest, isEnded);
    }

    details = node->find("ul.detail-list > :nth-child(2)");

    vector<Node *> categoriesNode = details->findAll("span:nth-child(1) > a");
    vector<Category> categories;

    string temp;
    for (Node *node : categoriesNode) {
      temp = node->getAttribute("href");
      temp = temp.substr(6, temp.length() - 7);

      for (const auto &pair : categoryText) {
        if (pair.second == temp) {
          categories.push_back(pair.first);
        }
      }
    }

    releaseMemory(categoriesNode);

    vector<Node *> authorNode = details->findAll("span:nth-child(2) > a");
    vector<string> authors;

    for (Node *node : authorNode) {
      authors.push_back(node->text());
    }

    releaseMemory(authorNode);
    delete details;

    Node *descriptionNode = node->find("div.book-intro > #intro-all");
    string description = descriptionNode->text();

    delete descriptionNode;

    auto pushChapters = [](Node *node, vector<Chapter> &chapters) {
      vector<Node *> chaptersGroupNode = node->findAll("ul");
      vector<Node *> chaptersNode;

      vector<Node *> temp;
      for (int i = chaptersGroupNode.size() - 1; i >= 0; --i) {
        temp = chaptersGroupNode[i]->findAll("li > a");
        chaptersNode.insert(chaptersNode.end(), temp.begin(), temp.end());
      }

      releaseMemory(chaptersGroupNode);

      for (Node *chapter : chaptersNode) {
        string id = strip(chapter->getAttribute("href"));
        RE2::GlobalReplace(&id, RE2("\\.html|\\/comic\\/.*\\/"), "");

        chapters.push_back({strip(chapter->getAttribute("title")), id});
      }

      releaseMemory(chaptersNode);
    };

    vector<Chapter> serial;
    vector<Chapter> extra;

    vector<Node *> chaptersNode;
    Node *tryAdult = node->tryFind("#__VIEWSTATE");

    if (tryAdult == nullptr) {
      chaptersNode = node->findAll("div.chapter-list");
    } else {
      string encodedElements = tryAdult->getAttribute("value");
      chaptersNode = (new Node(convert.to_bytes(lzstring::decompressFromBase64(
                          convert.from_bytes(encodedElements)))))
                         ->findAll("div.chapter-list");
      delete tryAdult;
    }

    if (chaptersNode.size() >= 1)
      pushChapters(chaptersNode.at(0), serial);

    if (chaptersNode.size() >= 2) {
      for (size_t i = 1; i < chaptersNode.size(); ++i) {
        pushChapters(chaptersNode.at(0), extra);
      }
    }
    releaseMemory(chaptersNode);

    return new DetailsManga(this, id, title, thumbnail, latest, authors,
                            isEnded, description, categories,
                            {serial, extra, id});
  }

  Manga *extractManga(Node *node) {
    Node *details = node->find("a");
    string title = details->getAttribute("title");
    string id = details->getAttribute("href");
    id = id = id.substr(7, id.length() - 8);

    Node *thumbnailNode = details->find("img");
    string thumbnail = "https:" + thumbnailNode->getAttribute("src");
    delete thumbnailNode;

    Node *isEndedNode = details->tryFind("span.fd");
    bool isEnded = isEndedNode != nullptr;
    delete isEndedNode;

    Node *latestNode = details->find("span.tt");
    string latest = latestNode->text();
    RE2::GlobalReplace(&latest, RE2(R"(更新至|\[完\]|\[全\]|共)"), "");
    latest = strip(latest);
    delete latestNode;

    delete details;

    return new Manga(this, id, title, thumbnail, latest, isEnded);
  }

  vector<string> decodeChapters(const string &encoded, const int &len1,
                                const int &len2, const string &valuesString) {
    string decoded = decompress(encoded, len1, len2, valuesString);

    RE2::GlobalReplace(&decoded, RE2(R"(SMH\.imgData\(|\)\.preInit\(\);)"), "");
    nlohmann::json data = nlohmann::json::parse(decoded);

    string baseUrl = "https://i.hamreus.com" + data["path"].get<string>();

    vector<string> result;
    for (const string &item : data["files"].get<vector<string>>()) {
      result.push_back(baseUrl + item);
    }

    return result;
  }
};