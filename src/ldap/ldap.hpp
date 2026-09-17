#pragma once

#include <memory>

#include "ber.hpp"

namespace ldap {
    enum class ParserErrorKind {
        NotEnoughData,
        InvalidData,
        Unknown
    };

    class ParserError {
      public:
        ParserError(ParserErrorKind);
        ParserError(ber::Error);

        const char* c_str();

        ParserErrorKind kind;
    };

    class Request {
      public:
        virtual ~Request() = 0;
        static Result<std::unique_ptr<Request>, ParserError> try_from(ber::Tlv);
        
        virtual ber::Tlv to_tlv() const = 0;
        virtual std::uint8_t get_tag() const = 0;

        int id;

      protected:
        Request(int id);
    };

    class Response {
      public:
        virtual ~Response() = 0;
        static Result<std::unique_ptr<Response>, ParserError> try_from(ber::Tlv);

        ber::Tlv to_tlv() const;

        virtual std::uint8_t get_tag() const = 0;

        int request_id;

      protected:
        Response(int request_id);

        virtual ber::Tlv inner_tlv() const = 0;
    };

    class InvalidRequest : public Request {
      public:
        ber::Tlv to_tlv() const override;
        std::uint8_t get_tag() const override;
    };
}