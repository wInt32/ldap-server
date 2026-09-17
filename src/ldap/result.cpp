#include "result.hpp"
#include "ber.hpp"

LdapResult::LdapResult(LdapResultCode code, std::string matched_dn, std::string error) {
    this->code = code;
    this->matched_dn = matched_dn;
    this->error = error;
}

ber::Tlv LdapResult::to_tlv() const {
    ber::BufEncoder encoder{};
    encoder.write_enum(static_cast<int>(this->code));
    encoder.write_str(this->matched_dn);
    encoder.write_str(this->error);
    return ber::Tlv{ber::Sequence, encoder.encode()};
}