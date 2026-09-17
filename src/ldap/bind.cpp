#include "bind.hpp"
#include "ber.hpp"
#include "ldap.hpp"

#include <cstddef>
#include <cstdint>

using namespace ldap;

BindRequest::BindRequest(int msg_id, int version, std::string username, std::string password)
    : Request{msg_id}, version{version}, username{username}, password{password} {}

static const std::size_t BIND_REQUEST_SEQ_LEN = 3;
static const std::uint8_t BIND_PASSWORD = 0x80;

Result<BindRequest, ParserError> BindRequest::try_from(int msg_id, ber::Tlv& tlv) {
    if (tlv.tag != BindRequest::TAG) TODO(); // error: invalid data

    ber::BufDecoder decoder(tlv.value);

    Result<int, ber::Error> ber_version = decoder.read_int();
    if (!ber_version.ok) TODO(); // error: invalid data

    Result<std::string, ber::Error> username = decoder.read_str();
    if (!username.ok) TODO(); // error: invalid data

    Result<std::string, ber::Error> password = decoder.read_str(BIND_PASSWORD);
    if (!password.ok) TODO(); // error: invalid data

    return BindRequest{msg_id, ber_version.value, username.value, password.value};
}

ber::Tlv BindRequest::to_tlv() const {
    TODO();
}

BindResponse::BindResponse(int request_id, LdapResultCode code, std::string matched_dn)
    : Response{request_id}, result{code, matched_dn} {}

ber::Tlv BindResponse::inner_tlv() const {
    ber::Tlv tlv = this->result.to_tlv();
    tlv.tag = BindResponse::TAG;
    return tlv;
}

BindRequest::~BindRequest() {}

std::uint8_t BindRequest::get_tag() const {
    return BindRequest::TAG;
}

BindResponse::~BindResponse() {}

std::uint8_t BindResponse::get_tag() const {
    return BindResponse::TAG;
}