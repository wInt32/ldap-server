#include "unbind.hpp"
#include "ldap.hpp"

using namespace ldap;

UnbindRequest::UnbindRequest(int msg_id) : Request{msg_id} {}

Result<UnbindRequest, ParserError> UnbindRequest::try_from(int msg_id, ber::Tlv& tlv) {
    if (tlv.tag != UnbindRequest::TAG) return {ParserErrorKind::InvalidData};
    if (tlv.len() != 0) return {ParserErrorKind::InvalidData};
    return UnbindRequest{msg_id};
}

std::uint8_t UnbindRequest::get_tag() const {
    return UnbindRequest::TAG;
}

ber::Tlv UnbindRequest::to_tlv() const {
    TODO();
}