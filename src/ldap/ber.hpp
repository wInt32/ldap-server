#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

#include "../util.hpp"

namespace ber {
    enum class ErrorKind {
        NotEnoughData,
        InvalidData,
        ValueNotAllowed,
        IO,
        Unimplemented,
        Unknown
    };
    class Error {
      public:
        Error(ErrorKind);
        Error(IOError);

        ErrorKind kind;
        union {
            IOError io;
        };
    };

    const std::uint8_t Bool = 0x01;
    const std::uint8_t Int = 0x02;
    const std::uint8_t Ostr = 0x04;
    const std::uint8_t Null = 0x05;
    const std::uint8_t Enum = 0x0a;
    const std::uint8_t Sequence = 0x30;
    const std::uint8_t Set = 0x31;

    class Tlv {
      public:
        std::vector<std::uint8_t> to_bytes() const;
        std::size_t len() const;

        std::uint8_t tag;
        std::vector<std::uint8_t> value;
    };
    
    class BufEncoder {
      public:
        BufEncoder();

        void write_len(std::size_t len);
        void write_tlv(const Tlv& tlv);
        void write_str(const std::string& value, std::uint8_t tag = ber::Ostr);
        void write_int(int value, std::uint8_t tag = ber::Int);
        void write_enum(int value, std::uint8_t tag = ber::Enum);
        void write_bool(bool value, std::uint8_t tag = ber::Bool);
        void write_null(std::uint8_t tag = ber::Null);
        std::vector<std::uint8_t> encode();

      private:
        std::vector<std::uint8_t> buf;
    };

    class BufDecoder {
      public:
        BufDecoder(const std::vector<std::uint8_t>&);

        bool empty();
        Result<std::size_t, Error> read_len();
        Result<Tlv, Error> read_tlv();
        Result<std::string, Error> read_str(std::uint8_t wanted_tag = ber::Ostr);
        Result<int, Error> read_int(std::uint8_t wanted_tag = ber::Int);
        Result<int, Error> read_enum(std::uint8_t wanted_tag = ber::Enum);
        Result<bool, Error> read_bool(std::uint8_t wanted_tag = ber::Bool);
        Optional<Error> read_null(std::uint8_t wanted_tag = ber::Null);

      private:
        bool has_at_least(std::size_t n) const;

        std::vector<std::uint8_t> buf;
        std::size_t pos;
    };

    Result<Tlv, Error> read_tlv(Stream&);
}

#define UNIVERSAL(x) (x)
#define APPLICATION(x) (x + (0b01 << 6))
#define CTXSPECIFIC(x) (x + (0b10 << 6))
#define PRIVATE(x) (x + (0b11 << 6))
#define PRIMITIVE(x) (x)
#define CONSTRUCTED(x) (x + (1 << 5))