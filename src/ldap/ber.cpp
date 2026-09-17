#include "ber.hpp"
#include <cstddef>
#include <string>

using namespace ber;

Error::Error(ErrorKind kind) : kind{kind} {}

Error::Error(IOError error) : kind{ErrorKind::IO}, io{error} {}

std::size_t Tlv::len() const {
    return this->value.size();
}

std::vector<std::uint8_t> Tlv::to_bytes() const {
    BufEncoder encoder{};
    encoder.write_tlv(*this);
    return encoder.encode();
}

BufEncoder::BufEncoder() {}
BufDecoder::BufDecoder(const std::vector<std::uint8_t>& src) : buf{src}, pos{0} {}

void BufEncoder::write_len(std::size_t len) {
    if (len < 127) {
        this->buf.push_back(len);
        return;
    }
    TODO(); // handle bigger lengths
}

void BufEncoder::write_tlv(const Tlv& tlv) {
    this->buf.push_back(tlv.tag);
    std::size_t len = tlv.len();
    this->write_len(len);
    this->buf.insert(this->buf.end(), tlv.value.cbegin(), tlv.value.cend());
}

std::vector<std::uint8_t> BufEncoder::encode() {
    return buf; // TODO: should we copy or not?
}

void BufEncoder::write_bool(bool value, std::uint8_t tag) {
    this->buf.reserve(this->buf.size()+3);
    this->buf.push_back(tag);
    this->write_len(1);
    this->buf.push_back(value ? 1 : 0);
}

void BufEncoder::write_str(const std::string& value, std::uint8_t tag) {
    this->buf.push_back(tag);
    std::size_t len = value.size();
    this->write_len(len);
    this->buf.insert(this->buf.end(), value.cbegin(), value.cend());
}

void BufEncoder::write_int(int value, std::uint8_t tag) {
    this->buf.push_back(tag);

    std::size_t value_len = 4;

    if (value < 0) TODO(); // TODO: handle negative integers
    if (value < 0x00ff0000) value_len--;
    if (value < 0x0000ff00) value_len--;
    if (value < 0x000000ff) value_len--;

    this->write_len(value_len);
    this->buf.reserve(this->buf.size()+value_len);

    std::uint8_t* raw_value = reinterpret_cast<std::uint8_t*>(&value);

    this->buf.insert(this->buf.end(), raw_value, raw_value+value_len);
}

void BufEncoder::write_enum(int value, std::uint8_t tag) {
    this->write_int(value, tag);
}

void BufEncoder::write_null(std::uint8_t tag) {
    this->buf.push_back(tag);
    this->buf.push_back(0);
}

const std::size_t MAX_ber_LEN = 2<<16;

bool BufDecoder::has_at_least(std::size_t n) const {
    if (n > this->buf.size()) return false;
    return n <= (this->buf.size() - this->pos);
}

bool BufDecoder::empty() {
    return !this->has_at_least(1);
}

Result<std::size_t, Error> BufDecoder::read_len() {
    if (!this->has_at_least(1)) return {ErrorKind::NotEnoughData};
    std::uint8_t b1 = this->buf[this->pos++];

    if (b1 < 127) return {b1};

    std::size_t n = b1 & 0b01111111;

    if (n > this->buf.size()-this->pos) return {ErrorKind::NotEnoughData};

    if (n == 4) {
        std::uint32_t val = 0;
        std::memcpy(&val, this->buf.data()+this->pos, 4);
        this->pos += 4;
        return {bswap_if_needed(val)};
    }

    std::size_t sz = 0;

    for (std::size_t i = 0; i < n && sz < MAX_ber_LEN; i++) {
        std::size_t tmp = this->buf[this->pos] << 8*(n-i);
        sz += tmp;
    }
    return sz;
}

Result<Tlv, Error> BufDecoder::read_tlv() {
    if (!this->has_at_least(1)) return {ErrorKind::NotEnoughData};
    std::uint8_t tag = this->buf[this->pos++];

    Result<std::size_t, Error> maybe_len = this->read_len();
    if (!maybe_len.ok) return maybe_len.error;
    std::size_t len = maybe_len.value;

    if (!this->has_at_least(len)) return {ErrorKind::NotEnoughData};
    auto first = this->buf.data()+this->pos;
    auto last = first + len;
    std::vector<std::uint8_t> value{first, last};
    this->pos += len;
    return Tlv{tag, value}; // TODO: avoid copy
}

Result<std::string, Error> BufDecoder::read_str(std::uint8_t wanted_tag) {
    if (!this->has_at_least(1)) return {ErrorKind::NotEnoughData};
    std::uint8_t tag = this->buf[this->pos++];
    if (tag != wanted_tag) return {ErrorKind::InvalidData};

    Result<std::size_t, Error> maybe_len = this->read_len();
    if (!maybe_len.ok) return maybe_len.error;
    std::size_t len = maybe_len.value;

    if (!this->has_at_least(len)) return {ErrorKind::NotEnoughData};
    auto first = this->buf.data()+this->pos;
    auto last = first + len;
    std::string res{first, last};
    this->pos += len;
    return res;
}

Result<int, Error> BufDecoder::read_int(std::uint8_t wanted_tag) {
    if (!this->has_at_least(1)) return {ErrorKind::NotEnoughData};
    std::uint8_t tag = this->buf[this->pos++];
    if (tag != wanted_tag) return {ErrorKind::InvalidData};

    Result<std::size_t, Error> maybe_len = this->read_len();
    if (!maybe_len.ok) return maybe_len.error;
    std::size_t len = maybe_len.value;

    if (len > 4) return {ErrorKind::ValueNotAllowed};
    if (!this->has_at_least(len)) return {ErrorKind::NotEnoughData};

    // TODO: handle negative integers
    std::uint32_t x = 0;

    for (std::size_t i = 0; i < len; i++) {
        x += this->buf[this->pos++] << 8*i;
    }

    return {static_cast<int>(x)};
}

Result<int, Error> BufDecoder::read_enum(std::uint8_t wanted_tag) {
    return this->read_int(wanted_tag);
}

Result<bool, Error> BufDecoder::read_bool(std::uint8_t wanted_tag) {
    if (!this->has_at_least(3)) return {ErrorKind::NotEnoughData};
    std::uint8_t tag = this->buf[this->pos++];
    if (tag != wanted_tag) return {ErrorKind::InvalidData};

    Result<std::size_t, Error> maybe_len = this->read_len();
    if (!maybe_len.ok) return maybe_len.error;
    std::size_t len = maybe_len.value;

    if (len != 1) return {ErrorKind::InvalidData};
    return this->buf[this->pos++] != 0;
}

Optional<Error> BufDecoder::read_null(std::uint8_t wanted_tag) {
    if (!this->has_at_least(1)) return {ErrorKind::NotEnoughData};
    std::uint8_t tag = this->buf[this->pos++];
    if (tag != wanted_tag) return {ErrorKind::InvalidData};

    Result<std::size_t, Error> maybe_len = this->read_len();
    if (!maybe_len.ok) return maybe_len.error;
    std::size_t len = maybe_len.value;

    if (len != 0) return {ErrorKind::InvalidData};
    return {};
}

static Result<std::size_t, Error> read_len(Stream& stream) {
    Result<std::uint8_t, IOError> maybe_len_b1 = stream.read_byte();
    if (!maybe_len_b1.ok) return {maybe_len_b1.error};
    std::uint8_t len_b1 = maybe_len_b1.value;

    std::size_t len = 0;
    if (len_b1 < 127) {
        len = len_b1;
    } else {
        std::size_t n = len_b1 & 127;

        Result<std::vector<std::uint8_t>, IOError> maybe_buf = stream.read_exact(n);
        if (!maybe_buf.ok) return {maybe_buf.error};

        BufDecoder decoder{maybe_buf.value};

        Result<std::size_t, Error> maybe_len = decoder.read_len();
        if (!maybe_len.ok) return maybe_len.error;

        len = maybe_len.value;
    }
    return {len};
}

Result<Tlv, Error> ber::read_tlv(Stream& stream) {
    Result<std::uint8_t, IOError> maybe_tag = stream.read_byte();
    if (!maybe_tag.ok) return {maybe_tag.error};
    std::uint8_t tag = maybe_tag.value;

    Result<std::size_t, Error> maybe_len = read_len(stream);
    if (!maybe_len.ok) return maybe_len.error;
    std::size_t len = maybe_len.value;

    Result<std::vector<std::uint8_t>, IOError> maybe_value = stream.read_exact(len);
    if (!maybe_value.ok) return {maybe_value.error};
    return Tlv{tag, std::move(maybe_value.value)};
}