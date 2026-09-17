#include <string>
#include <cstdint>

#include "ldap/db.hpp"
#include "util.hpp"
#include "client.hpp"

class LdapServer {
  public:
    LdapServer(std::uint16_t port, std::string db_path);
    ~LdapServer();

    void run();

  private:
    Optional<Client> accept();

    void error(const char *msg);
    Client do_accept();

    int server_fd; // TODO: add an abstraction layer?
    Database db;
    std::vector<ClientThread> threads;
};