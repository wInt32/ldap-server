#pragma once
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>
#include "search.hpp"
#include "../util.hpp"

class SearchFilter;
class Database;

class DatabaseCursor {
    friend Database;

    DatabaseCursor(std::size_t begin);

    std::size_t i;
};

struct User {
    std::string name;
    std::string uid;
    std::string email;
};

class Database {
  public:
    Database();
    Database(std::string file_path);

    DatabaseCursor start_search();
    Optional<User> search_one(SearchFilter *filter, DatabaseCursor& cursor);
  private:
    std::vector<User> users;
    std::mutex users_mtx;
    std::string file_path;
};
