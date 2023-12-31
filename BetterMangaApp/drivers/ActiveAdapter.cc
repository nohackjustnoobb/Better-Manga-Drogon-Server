#include "../../utils/Utils.hpp"
#include "../models/ActiveDriver.hpp"
#include "../models/BaseDriver.hpp"
#include "../models/Manga.hpp"

#include <SQLiteCpp/SQLiteCpp.h>
#include <algorithm>
#include <chrono>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>

class ActiveAdapter : public BaseDriver {
public:
  ActiveAdapter(ActiveDriver *driver) : driver(driver) {
    id = driver->id;
    proxySettings = driver->proxySettings;
    supportedCategories = driver->supportedCategories;
    version = driver->version;
    supportSuggestion = true;

    // Connect to database
    db = new SQLite::Database("../data/" + id + ".sqlite3",
                              SQLite::OPEN_NOMUTEX | SQLite::OPEN_READWRITE);

    // read the state
    std::ifstream inFile("../data/" + id + ".json");
    if (inFile.is_open()) {
      try {
        json state;
        inFile >> state;
        waitingList = state["waitingList"].get<vector<string>>();
      } catch (...) {
      }
    }
    inFile.close();

    // start a thread
    std::thread(&ActiveAdapter::updater, this).detach();

    // TODO proxy settings
  }

  vector<Manga *> getManga(vector<string> ids, bool showDetails) override {
    map<string, Chapters> cacheChapters;

    std::ostringstream oss;
    oss << "'";
    for (size_t i = 0; i < ids.size(); ++i) {
      oss << ids[i];
      if (i < ids.size() - 1)
        oss << "', '";
      if (showDetails)
        cacheChapters[ids[i]] = {};
    }
    oss << "'";

    SQLite::Statement query(*db, "SELECT * FROM MANGA WHERE ID in (" +
                                     oss.str() + ")");

    vector<Manga *> result;
    while (query.executeStep())
      result.push_back(toManga(query, showDetails));

    if (showDetails) {
      // Get the chapters
      SQLite::Statement chapterQuery(
          *db,
          "SELECT * FROM CHAPTER WHERE CHAPTERS_ID in (" + oss.str() + ")");

      while (chapterQuery.executeStep()) {
        Chapter chapter = {chapterQuery.getColumn(3).getString(),
                           chapterQuery.getColumn(1).getString()};

        if (chapterQuery.getColumn(4).getInt() == 1)
          cacheChapters[chapterQuery.getColumn(0).getString()].extra.push_back(
              chapter);
        else
          cacheChapters[chapterQuery.getColumn(0).getString()].serial.push_back(
              chapter);
      }

      // Get the extra data
      SQLite::Statement chaptersQuery(
          *db, "SELECT * FROM CHAPTERS WHERE MANGA_ID in (" + oss.str() + ")");
      while (chaptersQuery.executeStep())
        cacheChapters[chaptersQuery.getColumn(0).getString()].extraData =
            chaptersQuery.getColumn(1).getString();

      // assign it to the manga
      for (Manga *manga : result)
        ((DetailsManga *)manga)->chapters = cacheChapters[manga->id];
    }

    return result;
  }

  vector<string> getChapter(string id, string extraData) override {
    SQLite::Statement query(*db, "SELECT * FROM CHAPTER WHERE ID = ?");
    query.bind(1, id);
    query.executeStep();

    if (query.getColumn(5).isNull()) {
      vector<string> result = driver->getChapter(id, extraData);

      db->exec(fmt::format("UPDATE CHAPTER SET URLS = '{}' WHERE ID = '{}' AND "
                           "CHAPTERS_ID = '{}' AND IS_EXTRA = {}",
                           fmt::join(result, "|"), id,
                           query.getColumn(0).getString(),
                           query.getColumn(4).getString()));

      return result;
    } else {
      return split(query.getColumn(5).getString(), R"(\|)");
    }
  }

  vector<Manga *> getList(Category category, int page, Status status) override {
    string queryString = "SELECT * FROM MANGA ";
    if (status != Any)
      queryString += " WHERE IS_END = " + std::to_string(status == Ended);
    queryString += " ORDER BY -UPDATE_TIME LIMIT 50 OFFSET ?";

    SQLite::Statement query(*db, queryString);
    query.bind(1, (page - 1) * 50);
    vector<Manga *> result;
    while (query.executeStep())
      result.push_back(toManga(query));

    return result;
  }

  vector<string> getSuggestion(string keyword) override {
    SQLite::Statement query(*db,
                            "SELECT * FROM MANGA WHERE TITLE LIKE ? LIMIT 5");
    query.bind(1, "%" + keyword + "%");

    vector<string> result;
    while (query.executeStep())
      result.push_back(query.getColumn(2).getString());

    return result;
  }

  vector<Manga *> search(string keyword, int page) override {
    SQLite::Statement query(
        *db, "SELECT * FROM MANGA WHERE TITLE LIKE ? LIMIT 50 OFFSET ?");
    query.bind(1, "%" + keyword + "%");
    query.bind(2, (page - 1) * 50);

    vector<Manga *> result;
    while (query.executeStep())
      result.push_back(toManga(query));

    return result;
  }

  string useProxy(const string &destination, const string &genre) override {
    // TODO
    return destination;
  }

  bool checkOnline() override { return true; }

private:
  ActiveDriver *driver;
  SQLite::Database *db;
  int counter = 300;
  vector<string> waitingList;

  Manga *toManga(SQLite::Statement &query, bool showDetails = false) {
    if (showDetails) {
      vector<string> categoriesString =
          split(query.getColumn(6).getString(), R"(\|)");
      vector<Category> categories;
      for (string category : categoriesString)
        categories.push_back(stringToCategory(category));

      return new DetailsManga(
          this, query.getColumn(0).getString(), query.getColumn(2).getString(),
          query.getColumn(1).getString(), query.getColumn(7).getString(),
          split(query.getColumn(5).getString(), R"(\|)"),
          query.getColumn(4).getInt() == 1, query.getColumn(3).getString(),
          categories, {});
    } else {
      return new Manga(
          this, query.getColumn(0).getString(), query.getColumn(2).getString(),
          query.getColumn(1).getString(), query.getColumn(7).getString(),
          query.getColumn(4).getInt() == 1);
    }
  }

  void updater() {
    json state;
    string id;

    while (true) {
      // check if any updates every 5 minutes
      if (counter >= 300) {
        vector<PreviewManga> manga;
        try {
          manga = driver->getUpdates();
        } catch (...) {
          continue;
        }

        map<string, string> latestMap;
        std::ostringstream oss;
        oss << "'";
        for (size_t i = 0; i < manga.size(); ++i) {
          oss << manga[i].id;
          latestMap[manga[i].id] = manga[i].latest;
          if (i < manga.size() - 1)
            oss << "', '";
        }
        oss << "'";

        SQLite::Statement query(
            *db,
            fmt::format("SELECT * FROM MANGA WHERE ID in ({})", oss.str()));
        while (query.executeStep()) {
          id = query.getColumn(0).getString();

          // check if updated and not in the waiting list
          if (query.getColumn(7).getString().find(latestMap[id]) ==
                  std::string::npos &&
              std::find(waitingList.begin(), waitingList.end(), id) ==
                  waitingList.end()) {
            waitingList.insert(waitingList.begin(), id);
          }

          latestMap.erase(id);
        }

        // if not found in database
        for (const auto &pair : latestMap)
          waitingList.insert(waitingList.begin(), pair.first);

        // reset counter
        counter = 0;
      } else if (!waitingList.empty()) {
        id = waitingList.back();
        waitingList.pop_back();
        try {
          DetailsManga *manga =
              (DetailsManga *)driver->getManga({id}, true).at(0);

          std::ostringstream categories;
          for (size_t i = 0; i < manga->categories.size(); ++i) {
            categories << categoryToString(manga->categories[i]);
            if (i < manga->categories.size() - 1)
              categories << "|";
          }

          // update the manga info
          db->exec(fmt::format(
              "REPLACE INTO MANGA (ID, THUMBNAIL, TITLE, DESCRIPTION, "
              "IS_END, AUTHORS, CATEGORIES, LATEST, UPDATE_TIME) "
              "VALUES ('{}', '{}', '{}', '{}', {}, '{}', '{}', '{}', {})",
              manga->id, manga->thumbnail, manga->title, manga->description,
              (int)manga->isEnded, fmt::join(manga->authors, "|"),
              categories.str(), manga->latest,
              std::chrono::duration_cast<std::chrono::seconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count()));

          db->exec(fmt::format("REPLACE INTO CHAPTERS (MANGA_ID, "
                               "EXTRA_DATA) VALUES ('{}', '{}')",
                               manga->id, manga->chapters.extraData));

          auto updateChapter = [&](vector<Chapter> chapters, bool isExtra) {
            std::ostringstream oss;
            map<std::string, int> chaptersMap;
            oss << "'";
            for (size_t i = 0; i < chapters.size(); ++i) {
              oss << chapters[i].id;
              chaptersMap[chapters[i].id] = i;
              if (i < chapters.size() - 1)
                oss << "', '";
            }
            oss << "'";

            // remove the deleted chapter
            db->exec(fmt::format("DELETE FROM CHAPTER WHERE CHAPTERS_ID = '{}' "
                                 "AND IS_EXTRA = {} AND ID NOT IN ({})",
                                 manga->id, (int)isExtra, oss.str()));

            // insert only the new one
            SQLite::Statement query(*db, "SELECT ID FROM CHAPTER WHERE "
                                         "IS_EXTRA = ? AND CHAPTERS_ID = ?");
            query.bind(1, (int)isExtra);
            query.bind(2, manga->id);

            while (query.executeStep())
              chaptersMap.erase(query.getColumn(0).getString());

            for (const auto &pair : chaptersMap) {
              Chapter chapter = chapters[pair.second];
              db->exec(fmt::format(
                  "REPLACE INTO CHAPTER (CHAPTERS_ID, ID, IDX, TITLE, "
                  "IS_EXTRA) VALUES ('{}', '{}', {}, '{}', {})",
                  manga->id, chapter.id, chapters.size() - pair.second - 1,
                  chapter.title, (int)isExtra));
            }
          };

          updateChapter(manga->chapters.serial, false);
          updateChapter(manga->chapters.extra, true);
        } catch (...) {
          waitingList.insert(waitingList.begin(), id);
        }
      }

      // save state
      state["waitingList"] = waitingList;
      std::ofstream outFile("../data/" + this->id + ".json");
      outFile << state;
      outFile.close();

      counter += driver->timeout;
      std::this_thread::sleep_for(std::chrono::seconds(driver->timeout));
    }
  }
};