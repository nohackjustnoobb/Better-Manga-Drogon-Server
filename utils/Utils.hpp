#pragma once

#include "../BetterMangaApp/models/Common.hpp"
#include "TypeAliases.hpp"

#include <algorithm>
#include <cctype>
#include <cpr/cpr.h>
#include <list>
#include <locale>
#include <re2/re2.h>
#include <regex>

static cpr::Parameters mapToParameters(const map<string, string> &query) {
  cpr::Parameters parameters;

  for (const auto &pair : query) {
    parameters.Add({pair.first, pair.second});
  }

  return parameters;
}

static string urlEncode(const string &text) {
  std::ostringstream encoded;
  encoded.fill('0');
  encoded << std::uppercase << std::hex;

  for (char c : text) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded << c;
    } else if (c == ' ') {
      encoded << '+';
    } else {
      encoded << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
    }
  }

  string encodedText = encoded.str();

  RE2::GlobalReplace(&encodedText, RE2("\\+"), "%20");
  RE2::GlobalReplace(&encodedText, RE2("\\%7E"), "~");
  RE2::GlobalReplace(&encodedText, RE2("\\*"), "%2A");

  return encodedText;
}

static vector<string> split(string s, RE2 r) {
  vector<string> splits;

  // Use GlobalReplace to split the string
  RE2::GlobalReplace(&s, r, "\u001D");

  // Tokenize the modified string by \u001D
  std::istringstream tokenizer(s);
  std::string token;
  while (std::getline(tokenizer, token, '\u001D')) {
    splits.push_back(token);
  }

  return splits;
}

static string strip(const string &s) {
  string copy = s;
  RE2::GlobalReplace(&copy, RE2(R"(^\s+|\s+$)"), "");

  return copy;
}

static string decompress(const string &encoded, const int &len1,
                         const int &len2, const string &valuesString) {
  map<string, string> pairs;

  vector<string> values = split(valuesString, RE2("[|]"));
  std::function<string(int)> genKey = [&](int index) -> string {
    int lastChar = index % len1;

    return (index < len1 ? "" : genKey(index / len1)) +
           (char)(lastChar > 35
                      ? lastChar + 29
                      : "0123456789abcdefghijklmnopqrstuvwxyz"[lastChar]);
  };

  int i = len2;
  while (i--) {
    string key = genKey(i);
    pairs[key] = values.at(i) != "" ? values[i] : key;
  }

  string decoded = encoded;
  for (auto pair : pairs) {
    RE2::GlobalReplace(&decoded, RE2("\\b" + pair.first + "\\b"), pair.second);
  }

  return decoded;
}

static string categoryString[] = {
    "All",         "Passionate", "Love",    "Campus", "Yuri",   "Otokonoko",
    "BL",          "Adventure",  "Harem",   "SciFi",  "War",    "Suspense",
    "Speculation", "Funny",      "Fantasy", "Magic",  "Horror", "Ghosts",
    "History",     "FanFi",      "Sports",  "Hentai", "Mecha",  "Restricted"};

static string categoryToString(Category category) {
  return categoryString[category];
}

static Category stringToCategory(string category) {
  for (int i = 0; i < categoryString->length(); i++) {
    if (category == categoryString[i])
      return (Category)i;
  }

  return All;
}

template <typename T> static void releaseMemory(vector<T> vector) {
  for (T ptr : vector) {
    delete ptr;
  }
}
