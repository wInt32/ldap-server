#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <utility>

#include "db.hpp"
#include "../util.hpp"
#include "search.hpp"

Database::Database() {}

void db_error() {
    std::cerr << "error: invalid database format\n";
    std::exit(EXIT_FAILURE);
}

static User parse_user(std::string s) {
    std::size_t split1 = s.find(';');
    if (split1 == std::string::npos) db_error();

    std::size_t split2 = s.find(';', split1+1);
    if (split2 == std::string::npos) db_error();

    std::string name = s.substr(0, split1);
    std::string id_str = s.substr(split1+1, split2-split1-1);
    std::string email = s.substr(split2+1, s.length()-split2);

    return User {name, id_str, email};
}

Database::Database(std::string path) : file_path{path} {
    std::ifstream file = std::ifstream{this->file_path};
    if (!file) {
        std::cout << "error: could not open database file \'" << this->file_path << "\'\n";
        std::exit(1);
    }

    // TODO: don't load big files into memory, maybe use mmap or window?
    std::string line;
    while (std::getline(file, line)) {
        User u = parse_user(line);
        this->users.push_back(std::move(u));
    }
    std::cout << "db: found " << this->users.size() << " users\n";
}

DatabaseCursor::DatabaseCursor(std::size_t begin) : i{begin} {}

DatabaseCursor Database::start_search() {
    return {0};
}

Optional<User> Database::search_one(SearchFilter *filter, DatabaseCursor& cursor) {
    while (cursor.i < this->users.size()) {
        User user = this->users[cursor.i++];
        if (filter->matches(user)) {
            return {user};
        }
    }
    return {};
}