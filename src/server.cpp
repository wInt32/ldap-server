#include "server.hpp"

#include <cerrno>
#include <cstddef>
#include <future>
#include <iostream>

#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "ldap/db.hpp"
#include "util.hpp"
#include "signals.hpp"

static const int MAX_QUEUED_CONNECTIONS = 256;

LdapServer::LdapServer(std::uint16_t port, std::string db_path) 
  : db{db_path}  {

    // create a new IPv6 and IPv4 socket
    int res = ::socket(AF_INET6, SOCK_STREAM, 0);
    if (res < 0) this->error("socket()");
    this->server_fd = res;

    // bind the socket to the "any" address
    ::sockaddr_in6 sa{};
    sa.sin6_family = AF_INET6;
    sa.sin6_addr = in6addr_any;
    sa.sin6_port = ::htons(port);
    res = ::bind(this->server_fd, reinterpret_cast<sockaddr *>(&sa), sizeof(sa));
    if (res < 0) this->error("bind()");

    // set up signals and listen
    set_signal_handlers();
    
    res = ::listen(this->server_fd, MAX_QUEUED_CONNECTIONS);
    if (res < 0) this->error("listen()");
}

void LdapServer::error(const char *msg) {
    ::perror(msg);
    std::exit(1);
}

Optional<Client> LdapServer::accept() {
    // use poll() to enable receiving signals via the self pipe trick
    const std::size_t SERVER_FD = 0;
    const std::size_t SIGNALS = 1;

    ::pollfd events[2] = {};
    events[SERVER_FD] = {this->server_fd, POLLIN, 0};
    events[SIGNALS] = {signal_pipe[0], POLLIN, 0};

    const std::size_t events_len = sizeof(events)/sizeof(events[0]);

    do_poll: int n = ::poll(events, events_len, -1);
    if (n < 0) {
        if (errno == EAGAIN || errno == EINTR) goto do_poll;
        this->error("poll()");
    }

    short int server_events = events[SERVER_FD].revents;
    short int signal_events = events[SIGNALS].revents;

    if (server_events != 0) {
        // TODO: handle other event types
        return {this->do_accept()};
    }
    if (signal_events != 0) {
        int sig = 0;
        ::read(signal_pipe[0], &sig, sizeof(sig));
        std::cout << "Received signal: "  << sig << std::endl;
        return {};
    }

    std::cout << "Unknown error, exiting.\n";
    return {};
}

Client LdapServer::do_accept() {
    // ugly thing for portability
    ::sockaddr_storage sa;
    ::socklen_t sa_len = sizeof(sa);
    int res = ::accept(this->server_fd, reinterpret_cast<sockaddr *>(&sa), &sa_len);
    if (res < 0) this->error("accept()");
    return Client{res, this->db};
}

void LdapServer::run() {
    while (true) {
        // accept an incoming connection
        Optional<Client> maybe_client = this->accept();
        if (!maybe_client.has_value) break;
        Client& client = maybe_client.value;

        auto x = [](Client client){
            client.thread_entry();
        };

        // use this ugly thing to enable status checking
        std::packaged_task<void(Client)> task{x};
        std::future<void> ft = task.get_future();

        // run client logic on a new thread
        std::thread th = std::thread(std::move(task), std::move(client));

        // TODO: add heuristic to free unused spots?
        bool spot_found = false;
        for (ClientThread& client_thread : this->threads) {
            using namespace std::chrono_literals;

            // check if a client thread exited
            if (client_thread.future.wait_for(0s) == std::future_status::ready) {

                // join the old thread and reuse the spot
                spot_found = true;
                client_thread.thread.join();
                client_thread.future = std::move(ft);
                client_thread.thread = std::move(th);
                break;
            }
        }

        // allocate a new spot
        if (!spot_found) this->threads.emplace_back(std::move(ft), std::move(th));
    }
}

LdapServer::~LdapServer() {
    // stop accepting new clients
    ::close(this->server_fd);

    shutdown_pipe.write(1);

    // wait for threads to shutdown
    for (ClientThread& cth : this->threads) {
        cth.thread.join();
    }
}