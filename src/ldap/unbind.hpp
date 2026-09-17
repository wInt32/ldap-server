#include "ldap.hpp"
#include "ber.hpp"

namespace ldap {
    class UnbindRequest : public Request {
      public:
        UnbindRequest(int msg_id);

        static Result<UnbindRequest, ParserError> try_from(int msg_id, ber::Tlv& tlv);

        static const std::uint8_t TAG = APPLICATION(2);

        ber::Tlv to_tlv() const override;
        std::uint8_t get_tag() const override;
    };
}
