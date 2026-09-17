#pragma once
#include "ldap/bind.hpp"
#include "ldap/db.hpp"
#include "ldap/ldap.hpp"

#include <cstddef>
#include <future>

class ClientThread {
  public:
    ClientThread(std::future<void> ft, std::thread th);

    std::future<void> future;
    std::thread thread;
};

class Client {
  public:
    Client(int fd, Database& db);
    Client(const Client&) = delete;
    Client(Client&&);
    Client operator=(const Client&) = delete;
    Client operator=(Client&&);
    ~Client();

    void thread_entry();

  private:
    void on_request(ldap::Request& request);
    void do_bind(ldap::BindRequest& bind_request);
    void do_search(ldap::SearchRequest& search_request);

    Result<std::unique_ptr<ldap::Request>, ldap::ParserError> receive();
    void send(const ldap::Response&);

    int fd;
    bool active;
    SocketStream stream;
    Database& db;
};
