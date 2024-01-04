#pragma once

#include <string>

class Manga;

class DetailsManga;

struct PreviewManga {
  std::string id;
  std::string latest;
};

enum Status {
  Any,
  OnGoing,
  Ended,
};

enum Category {
  All,
  Passionate,
  Love,
  Campus,
  Yuri,
  Otokonoko,
  BL,
  Adventure,
  Harem,
  SciFi,
  War,
  Suspense,
  Speculation,
  Funny,
  Fantasy,
  Magic,
  Horror,
  Ghosts,
  History,
  FanFi,
  Sports,
  Hentai,
  Mecha,
  Restricted,
};
