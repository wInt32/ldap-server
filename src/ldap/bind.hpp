#pragma once
#include "ber.hpp"
#include "ldap.hpp"
#include "result.hpp"

namespace ldap {
  class BindRequest : public Request {
    public:
      BindRequest(int msg_id, int version, std::string username, std::string password);
      ~BindRequest() override;

      static Result<BindRequest, ParserError> try_from(int msg_id, ber::Tlv& tlv);

      static const std::uint8_t TAG = APPLICATION(CONSTRUCTED(0));

      std::uint8_t get_tag() const override;
      ber::Tlv to_tlv() const override;

      int version;
      std::string username;
      std::string password;
  };

  class BindResponse : public Response {
    public:
      BindResponse(int request_id, LdapResultCode code, std::string matched_dn);
      ~BindResponse() override;

      static const std::uint8_t TAG = APPLICATION(CONSTRUCTED(1));

      std::uint8_t get_tag() const override;

      LdapResult result;

    private:
      BindResponse();

      ber::Tlv inner_tlv() const override;
  };
}