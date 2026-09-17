#include "ldap.hpp"
#include "../util.hpp"
#include "ber.hpp"
#include "bind.hpp"
#include "search.hpp"
#include "unbind.hpp"
#include <memory>

static const char* REASON_INVALID_DATA = "invalid data";
static const char* REASON_NOT_ENOUGH_DATA = "not enough data";
static const char* REASON_UNKNOWN = "unknown error";

using namespace ldap;

ParserError::ParserError(ParserErrorKind kind) : kind{kind} {}

static ParserErrorKind map_ber_error(ber::ErrorKind kind) {
    switch (kind) {
        case ber::ErrorKind::NotEnoughData: return ParserErrorKind::NotEnoughData;
        case ber::ErrorKind::InvalidData: return ParserErrorKind::InvalidData;
        case ber::ErrorKind::ValueNotAllowed: return ParserErrorKind::Unknown;
        case ber::ErrorKind::IO: return ParserErrorKind::Unknown;
        case ber::ErrorKind::Unimplemented: return ParserErrorKind::Unknown;
        case ber::ErrorKind::Unknown: return ParserErrorKind::Unknown;
    }
    return ParserErrorKind::Unknown;
}

ParserError::ParserError(ber::Error e) : kind{map_ber_error(e.kind)} {}

const char* ParserError::c_str() {
    switch (this->kind) {
        case ParserErrorKind::InvalidData: return REASON_INVALID_DATA;
        case ParserErrorKind::NotEnoughData: return REASON_NOT_ENOUGH_DATA;
        case ParserErrorKind::Unknown: return REASON_UNKNOWN;
        default: TODO();
    }
}

Request::Request(int msg_id) : id{msg_id} {}
Request::~Request() {}

Response::Response(int request_id) : request_id{request_id} {}
Response::~Response() {}

template<typename T>
static Result<std::unique_ptr<Request>, ParserError> make_request(int msg_id, ber::Tlv& tlv) {
    auto&& r = T::try_from(msg_id, tlv);
    if (!r.ok) return r.error;
    return {std::make_unique<T>(std::move(r.value))};
}

Result<std::unique_ptr<Request>, ParserError> Request::try_from(ber::Tlv tlv) {
    if (tlv.tag != ber::Sequence) return {ParserErrorKind::InvalidData};

    std::vector<std::uint8_t>& content = tlv.value;

    ber::BufDecoder decoder{content};

    Result<int, ber::Error> maybe_msg_id = decoder.read_int();
    if (!maybe_msg_id.ok) return {maybe_msg_id.error};
    int msg_id = maybe_msg_id.value;

    Result<ber::Tlv, ber::Error> maybe_op_tlv = decoder.read_tlv();
    if (!maybe_op_tlv.ok) return {maybe_op_tlv.error};
    ber::Tlv op_tlv = maybe_op_tlv.value;

    switch (op_tlv.tag) {
        case BindRequest::TAG: return make_request<BindRequest>(msg_id, op_tlv);
        case SearchRequest::TAG: return make_request<SearchRequest>(msg_id, op_tlv);
        case UnbindRequest::TAG: return make_request<UnbindRequest>(msg_id, op_tlv);
    }
    return {ParserErrorKind::Unknown};
}

ber::Tlv Response::to_tlv() const {
    ber::BufEncoder encoder{};
    encoder.write_int(this->request_id);
    encoder.write_tlv(this->inner_tlv());
    return {ber::Sequence, encoder.encode()};
}

std::uint8_t InvalidRequest::get_tag() const {
    std::abort(); // this request doesn't really exist
}

ber::Tlv InvalidRequest::to_tlv() const {
    std::abort(); // this request doesn't really exist
}