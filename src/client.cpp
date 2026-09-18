#include "client.hpp"

#include <future>
#include <memory>
#include <unistd.h>

#include "ldap/ber.hpp"
#include "ldap/db.hpp"
#include "ldap/ldap.hpp"
#include "ldap/bind.hpp"
#include "ldap/result.hpp"
#include "ldap/unbind.hpp"
#include "ldap/search.hpp"
#include "util.hpp"

using namespace ldap;

ClientThread::ClientThread(std::future<void> ft, std::thread th) : future{std::move(ft)}, thread{std::move(th)} {}

Client::Client(int fd, Database& db) : fd{fd}, stream{fd}, db{db} {}

Client::Client(Client&& other) : fd{other.fd}, db{other.db} {
    other.fd = -1;
    this->stream = std::move(other.stream);
}

Client::~Client() {
    if (fd >= 0) ::close(this->fd);
}

void Client::thread_entry() {
    this->active = true;

    while (this->active) {
        Result<std::unique_ptr<Request>, ParserError> maybe_req = this->receive();
        if (!maybe_req.ok) {
            if (maybe_req.error.kind != ParserErrorKind::Unknown) {
                std::cout << "error: " << maybe_req.error.c_str() << '\n';
            }
            return;
        }
        this->on_request(*maybe_req.value.get());
    }
}

void Client::on_request(Request& req) {
    if (dynamic_cast<InvalidRequest*>(&req)) {
        BindResponse res{req.id, LdapResultCode::ProtocolError, ""};
        this->send(res);
        this->active = false;
        return;
    }
    switch (req.get_tag()) {
        case BindRequest::TAG: return this->do_bind(static_cast<BindRequest&>(req));
        case SearchRequest::TAG: return this->do_search(static_cast<SearchRequest&>(req));
        case UnbindRequest::TAG: this->active = false;
    }
}

void Client::do_bind(BindRequest& bind) {
    std::cout << "ldap: Bind requested for " << bind.username << "\n";
    BindResponse res{bind.id, LdapResultCode::Success, bind.username};
    this->send(res);
}

void Client::do_search(SearchRequest& search) {
    std::cout << "ldap: Search started\n";

    DatabaseCursor cur = this->db.start_search();
    while (true) {
        Optional<User> user = this->db.search_one(search.options.filter.get(), cur);
        if (!user.has_value) break;
        SearchResultEntry search_entry = {search.id, search.options.base_object, std::vector<SearchAttr>{
            {"cn", {user.value.name}},
            {"email", {user.value.email}},
            {"uid", {user.value.uid}}
        }};
        this->send(search_entry);
    }

    std::cout << "ldap: Search ended\n";
    SearchResultDone res{search.id, {LdapResultCode::Success, search.options.base_object}};
    this->send(res);
}

void Client::send(const Response& response) {
    Optional<IOError> e = this->stream.write(response.to_tlv().to_bytes());
    if (e.has_value) this->active = false;
}

Result<std::unique_ptr<Request>, ParserError> Client::receive() {
    Result<ber::Tlv, ber::Error> envelope = ber::read_tlv(stream);
    if (!envelope.ok) return {envelope.error};
    return Request::try_from(envelope.value);
}