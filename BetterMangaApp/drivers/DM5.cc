#include "../../utils/Converter.hpp"
#include "../../utils/Node.hpp"
#include "../../utils/Utils.hpp"
#include "../models/BaseDriver.hpp"
#include "../models/Manga.hpp"

#include <cpr/cpr.h>
#include <mutex>
#include <thread>

class DM5 : public BaseDriver {
public:
  DM5() {
    id = "DM5";
    supportSuggestion = true;
    recommendedChunkSize = 10;
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

    vector<std::thread> threads;
    std::mutex mutex;
    auto fetchId = [&](const string &id, vector<Manga *> &result) {
      cpr::Response r = cpr::Get(cpr::Url{baseUrl + "manhua-" + id}, header,
                                 cpr::Cookies{{"isAdult", "1"}});
      Node *body = new Node(r.text);

      Manga *manga = extractDetails(body, id, showDetails);
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

    return result;
  };

  vector<string> getChapter(string id, string extraData) override {
    cpr::Response r =
        cpr::Get(cpr::Url{"https://www.manhuaren.com/m" + id}, header);

    vector<string> result;
    re2::StringPiece input(r.text);
    string encoded, valuesString, len1String, len2String;

    if (RE2::PartialMatch(
            input,
            RE2(R"((?m)^.*\}\(\'(.*)\',(\d*),(\d*),\'([\w|\+|\/|=]*)\'.*$)"),
            &encoded, &len1String, &len2String, &valuesString)) {
      int len1, len2;
      len1 = std::stoi(len1String);
      len2 = std::stoi(len2String);

      result = decodeChapters(encoded, len1, len2, valuesString);
    }

    return result;
  };

  vector<Manga *> getList(Category category, int page, Status status) override {
    cpr::Response r =
        cpr::Get(cpr::Url{baseUrl + "manhua-list-tag" +
                          std::to_string(categoryId[category]) + "-st" +
                          std::to_string(status) + "-p" + std::to_string(page)},
                 header);

    vector<Manga *> result;

    Node *body = new Node(r.text);
    vector<Node *> items = body->findAll("ul.col7.mh-list > li > div.mh-item");
    for (Node *node : items) {
      result.push_back(extractManga(node));
    }
    releaseMemory(items);
    delete body;

    return result;
  };

  vector<string> getSuggestion(string keyword) override {
    cpr::Response r =
        cpr::Get(cpr::Url{baseUrl + "search.ashx?t=" +
                          urlEncode(chineseConverter.toSimplified(keyword))},
                 header);

    vector<string> result;

    Node *body = new Node(r.text);
    for (Node *node : body->findAll("span.left")) {
      result.push_back(node->text());
    }
    delete body;

    return result;
  };

  vector<Manga *> search(string keyword, int page) override {
    cpr::Response r =
        cpr::Get(cpr::Url{baseUrl + "search?title=" +
                          urlEncode(chineseConverter.toSimplified(keyword)) +
                          "&page=" + std::to_string(page)},
                 header);

    vector<Manga *> result;
    Node *body = new Node(r.text);

    Node *huge = body->find("div.banner_detail_form");
    if (huge != nullptr) {
      // extract thumbnail
      Node *thumbnailNode = huge->find("img");
      string thumbnail = thumbnailNode->getAttribute("src");
      delete thumbnailNode;

      // extract title & id
      Node *titleNode = huge->find("p.title > a");
      string title = titleNode->text();
      string id = titleNode->getAttribute("href");
      id = id = id.substr(8, id.length() - 9);
      delete titleNode;

      // extract isEnded
      Node *isEndedNode = huge->find("span.block > span");
      bool isEnded = isEndedNode->text() != "连载中";
      delete isEndedNode;

      // extract latest
      Node *latestNode = huge->find("div.bottom > a.btn-2");
      string latest = strip(latestNode->getAttribute("title"));
      size_t pos = latest.find(title + " ");
      if (pos != std::string::npos) {
        latest.replace(pos, (title + " ").length(), "");
      }
      delete latestNode;

      result.push_back(new Manga(this, id, title, thumbnail, latest, isEnded));
    }

    for (Node *node : body->findAll("ul.col7.mh-list > li > div.mh-item")) {
      result.push_back(extractManga(node));
    }
    delete body;

    return result;
  };

  bool checkOnline() override {
    return cpr::Get(cpr::Url{baseUrl}, cpr::Timeout{5000}).status_code != 0;
  };

private:
  map<Category, int> categoryId = {
      {All, 0},     {Passionate, 31}, {Love, 26},      {Campus, 1},
      {Yuri, 3},    {BL, 27},         {Adventure, 2},  {Harem, 8},
      {SciFi, 25},  {War, 12},        {Suspense, 17},  {Speculation, 33},
      {Funny, 37},  {Fantasy, 14},    {Magic, 15},     {Horror, 29},
      {Ghosts, 20}, {History, 4},     {FanFi, 30},     {Sports, 34},
      {Hentai, 36}, {Mecha, 40},      {Restricted, 61}};
  std::map<std::string, Category> categoryText = {
      {"rexue", Passionate},    {"aiqing", Love},
      {"xiaoyuan", Campus},     {"baihe", Yuri},
      {"caihong", BL},          {"maoxian", Adventure},
      {"hougong", Harem},       {"kehuan", SciFi},
      {"zhanzheng", War},       {"xuanyi", Suspense},
      {"zhentan", Speculation}, {"gaoxiao", Funny},
      {"qihuan", Fantasy},      {"mofa", Magic},
      {"kongbu", Horror},       {"dongfangshengui", Ghosts},
      {"lishi", History},       {"tongren", FanFi},
      {"jingji", Sports},       {"jiecao", Hentai},
      {"jizhan", Mecha},        {"list-tag61", Restricted}};
  string baseUrl = "https://www.dm5.com/";
  cpr::Header header = cpr::Header{{{"Accept-Language", "en-US,en;q=0.5"}}};
  Converter chineseConverter;

  Manga *extractManga(Node *node) {
    // extract title & id
    Node *titleNode = node->find("h2.title > a");
    string title = titleNode->text();
    string id = titleNode->getAttribute("href");
    id = id.substr(8, id.length() - 9);

    // release memory allocated
    delete titleNode;

    // extract thumbnail
    Node *thumbnailNode = node->find("p.mh-cover");
    string thumbnailStyle = thumbnailNode->getAttribute("style");
    string thumbnail;

    re2::StringPiece url;
    if (RE2::PartialMatch(thumbnailStyle, RE2("url\\(([^)]+)\\)"), &url)) {
      thumbnail = string(url.data(), url.size());
    }

    // release memory allocated
    delete thumbnailNode;

    // extract isEnded & latest
    Node *latestNode = node->find("p.chapter");
    Node *temp = latestNode->find("span");
    bool isEnded = temp->text() == "完结";

    // release memory allocated
    delete temp;

    temp = latestNode->find("a");
    string latest = strip(temp->text());

    // release memory allocated
    delete temp;

    return new Manga(this, id, title, thumbnail, latest, isEnded);
  }

  Manga *extractDetails(Node *node, const string &id, const bool showDetails) {
    Node *infoNode = node->find("div.info");

    if (infoNode == nullptr)
      throw "Cannot extract details";

    // extract title
    Node *titleNode = infoNode->find("p.title");
    string title = strip(titleNode->content());

    delete titleNode;

    // extract authors
    vector<Node *> authorNode = infoNode->findAll("p.subtitle > a");
    vector<string> authors;

    for (Node *node : authorNode) {
      authors.push_back(node->text());
    }

    // release memory allocated
    releaseMemory(authorNode);

    // extract thumbnail
    Node *thumbnailNode = node->find("div.cover > img");
    string thumbnail = thumbnailNode->getAttribute("src");

    delete thumbnailNode;

    // extract isEnded & latest
    Node *latestNode = node->find("div.detail-list-title > span.s > span > a");
    string latest = strip(latestNode->text());

    // release memory allocated
    delete latestNode;

    Node *tipNode = node->find("p.tip");
    Node *isEndedNode = tipNode->find("span.block > span");
    bool isEnded = isEndedNode->text() != "连载中";

    delete isEndedNode;

    if (!showDetails) {
      delete tipNode;
      delete infoNode;

      return new Manga(this, id, title, thumbnail, latest, isEnded);
    }

    Node *descriptionNode = infoNode->find("p.content");
    string description = descriptionNode->text();
    size_t pos = description.find("[+展开]");
    if (pos != std::string::npos) {
      description.replace(pos, 12, "");
    }
    pos = description.find("[-折叠]");
    if (pos != std::string::npos) {
      description.replace(pos, 9, "");
    }

    delete infoNode;

    vector<Node *> categoriesNode = tipNode->findAll("a");
    vector<Category> categories;

    string temp;
    for (Node *node : categoriesNode) {
      temp = node->getAttribute("href");
      temp = temp.substr(8, temp.length() - 9);

      if (categoryText.find(temp) != categoryText.end()) {
        categories.push_back(categoryText[temp]);
      }
    }

    releaseMemory(categoriesNode);
    delete tipNode;

    auto pushChapters = [](Node *node, vector<Chapter> &chapters) {
      vector<Node *> chaptersNode = node->findAll("a");
      for (Node *chapter : chaptersNode) {
        string id = strip(chapter->getAttribute("href"));
        id = id.substr(2, id.length() - 3);

        chapters.push_back({strip(chapter->content()), id});
      }

      releaseMemory(chaptersNode);
    };

    vector<Chapter> serial;
    vector<Chapter> extra;

    Node *serialNode = node->find("#detail-list-select-1");
    if (serialNode != nullptr)
      pushChapters(serialNode, serial);
    delete serialNode;

    Node *extraNode = node->find("#detail-list-select-2");
    if (extraNode != nullptr)
      pushChapters(extraNode, serial);
    delete extraNode;

    extraNode = node->find("#detail-list-select-3");
    if (extraNode != nullptr)
      pushChapters(extraNode, serial);
    delete extraNode;

    return new DetailsManga(this, id, title, thumbnail, latest, authors,
                            isEnded, description, categories,
                            {serial, extra, ""});
  }

  vector<string> decodeChapters(const string &encoded, const int &len1,
                                const int &len2, const string &valuesString) {
    string decoded = decompress(encoded, len1, len2, valuesString);

    RE2::GlobalReplace(&decoded, RE2(R"((.+\[\\')|\];)"), "");

    vector<string> result = split(decoded, RE2(R"(\\',\\'|\\')"));

    return result;
  }
};
