#include "search.hpp"
#include "ldap.hpp"
#include "result.hpp"
#include <cstddef>
#include <memory>

#include "ber.hpp"

using namespace ldap;

template<typename T>
static Result<std::unique_ptr<SearchFilter>, ParserError> make_filter(ber::Tlv& tlv) {
    auto r = T::try_from(tlv);
    if (!r.ok) return r.error;
    return {std::make_unique<T>(std::move(r.value))};
}

Result<std::unique_ptr<SearchFilter>, ParserError> SearchFilter::try_from(ber::Tlv tlv) {
    switch (tlv.tag) {
        case FilterEqMatch::TAG: return make_filter<FilterEqMatch>(tlv);
    }
    return {ParserErrorKind::Unknown};
}

FilterPresent::FilterPresent(std::string attr) : attr{attr} {}

Result<FilterPresent, ParserError> FilterPresent::try_from(ber::Tlv& tlv) {
    ber::BufDecoder decoder{tlv.value};

    Result<std::string, ber::Error> maybe_attr = decoder.read_str();
    if (maybe_attr.ok) return FilterPresent{maybe_attr.value};
    return {maybe_attr.error};
}

bool FilterPresent::matches(const User& user) {
    TODO();
}

FilterEqMatch::FilterEqMatch(std::string attr, std::string val) : attr{attr}, val{val} {}

Result<FilterEqMatch, ParserError> FilterEqMatch::try_from(ber::Tlv& tlv) {
    if (tlv.tag != FilterEqMatch::TAG) return {ParserErrorKind::InvalidData};
    ber::BufDecoder decoder{tlv.value};

    Result<std::string, ber::Error> maybe_attr = decoder.read_str();
    if (!maybe_attr.ok) return {maybe_attr.error};

    Result<std::string, ber::Error> maybe_val = decoder.read_str();
    if (!maybe_val.ok) return {maybe_val.error};

    return FilterEqMatch{maybe_attr.value, maybe_val.value};
}

bool FilterEqMatch::matches(const User& user) {
    if (this->attr == "email") return this->val == user.email;
    if (this->attr == "cn") return this->val == user.name;
    if (this->attr == "uid") return this->val == user.uid;
    return false;
}

SearchRequest::SearchRequest(int msg_id, SearchOptions options)
    : Request{msg_id}, options{std::move(options)} {}


Result<SearchRequest, ParserError> SearchRequest::try_from(int msg_id, ber::Tlv& tlv) {
    if (tlv.tag != SearchRequest::TAG) return {ParserErrorKind::InvalidData};
    ber::BufDecoder decoder{tlv.value};

    Result<std::string, ber::Error> maybe_base_object = decoder.read_str();
    if (!maybe_base_object.ok) return {maybe_base_object.error};
    std::string base_object = maybe_base_object.value;

    Result<int, ber::Error> maybe_scope = decoder.read_enum();
    if (!maybe_scope.ok) return {maybe_scope.error};
    SearchScope scope = static_cast<SearchScope>(maybe_scope.value);

    Result<int, ber::Error> maybe_deref_aliases = decoder.read_enum();
    if (!maybe_deref_aliases.ok) return {maybe_deref_aliases.error};
    SearchDerefAliases deref_aliases = static_cast<SearchDerefAliases>(maybe_deref_aliases.value);
    
    Result<int, ber::Error> maybe_size_limit = decoder.read_int();
    if (!maybe_size_limit.ok) return {maybe_size_limit.error};
    int size_limit = maybe_size_limit.value;

    Result<int, ber::Error> maybe_time_limit = decoder.read_int();
    if (!maybe_time_limit.ok) return {maybe_time_limit.error};
    int time_limit = maybe_time_limit.value;

    Result<bool, ber::Error> maybe_attrs_only = decoder.read_bool();
    if (!maybe_attrs_only.ok) return {maybe_attrs_only.error};
    bool attrs_only = maybe_attrs_only.value;

    Result<ber::Tlv, ber::Error> maybe_filter_tlv = decoder.read_tlv();
    if (!maybe_filter_tlv.ok) return {maybe_filter_tlv.error};
    Result<std::unique_ptr<SearchFilter>, ParserError> maybe_filter = SearchFilter::try_from(maybe_filter_tlv.value);
    if (!maybe_filter.ok) return {maybe_filter.error};
    std::unique_ptr<SearchFilter> filter = std::move(maybe_filter.value);

    Result<ber::Tlv, ber::Error> maybe_attrs_tlv = decoder.read_tlv();
    if (!maybe_attrs_tlv.ok) return {maybe_attrs_tlv.error};
    if (maybe_attrs_tlv.value.tag != ber::Sequence) return {ParserErrorKind::InvalidData};

    std::vector<std::string> attrs{};
    if (maybe_attrs_tlv.value.len() > 0) {
        ber::BufDecoder attr_decoder{maybe_attrs_tlv.value.value};
        while (true) {
            Result<std::string, ber::Error> maybe_attr = attr_decoder.read_str();
            if (!maybe_attr.ok) {
                if (attr_decoder.empty()) break;
                return {maybe_attr.error};
            }
            attrs.push_back(maybe_attr.value);
            std::cout << "ldap: found attr: " << maybe_attr.value << '\n';
        }
    }

    SearchOptions opts {
        base_object,
        scope,
        deref_aliases,
        static_cast<size_t>(size_limit),
        static_cast<size_t>(time_limit),
        attrs_only,
        std::move(filter),
        attrs
    };        
    return {SearchRequest{msg_id, std::move(opts)}};
}

SearchResultEntry::SearchResultEntry(int request_id, std::string object_name, std::vector<SearchAttr> entry)
    : Response{request_id}, object_name{object_name}, entry{entry} {}

SearchResultDone::SearchResultDone(int request_id, LdapResult res) : Response{request_id}, result{res} {}

SearchRequest::~SearchRequest() {}

std::uint8_t SearchRequest::get_tag() const {
    return SearchRequest::TAG;
}

ber::Tlv SearchRequest::to_tlv() const {
    TODO();
}

SearchResultEntry::~SearchResultEntry() {}

std::uint8_t SearchResultEntry::get_tag() const {
    return SearchResultEntry::TAG;
}

static ber::Tlv encode_attr(const SearchAttr& attr) {
    ber::BufEncoder attr_encoder{};
    attr_encoder.write_str(attr.type);
    ber::BufEncoder attr_val_encoder{};
    for (const std::string& val : attr.value) {
        attr_val_encoder.write_str(val);
    }
    attr_encoder.write_tlv({ber::Set, attr_val_encoder.encode()});
    return {ber::Sequence, attr_encoder.encode()};
}

static ber::Tlv encode_attrs(const std::vector<SearchAttr>& attrs) {
    ber::BufEncoder encoder{};
    for (const SearchAttr& attr : attrs) {
        encoder.write_tlv(encode_attr(attr));
    }
    return {ber::Sequence, encoder.encode()};
}

ber::Tlv SearchResultEntry::inner_tlv() const {
    ber::BufEncoder encoder{};
    encoder.write_str(this->object_name);
    encoder.write_tlv(encode_attrs(this->entry));
    return {SearchResultEntry::TAG, encoder.encode()};
}

SearchResultDone::~SearchResultDone() {}

std::uint8_t SearchResultDone::get_tag() const {
    return SearchResultDone::TAG;
}

ber::Tlv SearchResultDone::inner_tlv() const {
    ber::Tlv tlv = this->result.to_tlv();
    tlv.tag = SearchResultDone::TAG;
    return tlv;
}