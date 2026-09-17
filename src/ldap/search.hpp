#pragma once
#include "ber.hpp"
#include "ldap.hpp"
#include "result.hpp"
#include "db.hpp"
#include <cstddef>

enum class SearchScope : int {
    BaseObject = 0,
    SingleLevel = 1,
    WholeSubtree = 2
};

enum class SearchDerefAliases : int {
    NeverDerefAliases = 0,
    DerefInSearching = 1,
    DerefFindingBaseObj = 2,
    AlwaysDerefAliases = 3
};

class User;

class SearchFilter {
  public:
    static Result<std::unique_ptr<SearchFilter>, ldap::ParserError> try_from(ber::Tlv tlv);

    virtual bool matches(const User& user) = 0;
};

class FilterAnd : public SearchFilter {
  public:
    static const std::uint8_t TAG = CONSTRUCTED(CTXSPECIFIC(0));
};
class FilterOr : public SearchFilter {
  public:
    static const std::uint8_t TAG = CONSTRUCTED(CTXSPECIFIC(1));
};
class FilterNot : public SearchFilter {
  public:
    static const std::uint8_t TAG = CONSTRUCTED(CTXSPECIFIC(2));
};

class FilterEqMatch : public SearchFilter {
  public:
    FilterEqMatch(std::string attr, std::string val);

    static const std::uint8_t TAG = CONSTRUCTED(CTXSPECIFIC(3));

    static Result<FilterEqMatch, ldap::ParserError> try_from(ber::Tlv& tlv);

    bool matches(const User&) override;
    
    std::string attr;
    std::string val;
};

class FilterSubstr : public SearchFilter {
    // TODO
};

class FilterGeq : public SearchFilter {
    // TODO
};

class FilterLeq : public SearchFilter {
    // TODO
};

class FilterPresent : public SearchFilter {
  public:
    FilterPresent(std::string);

    static Result<FilterPresent, ldap::ParserError> try_from(ber::Tlv& tlv);

    bool matches(const User& user) override;

    std::string attr;
};
class FilterApproxMatch : public SearchFilter {
    // TODO
};

namespace ldap {
    struct SearchOptions {
        std::string base_object;
        SearchScope scope;
        SearchDerefAliases deref_aliases;
        std::size_t size_limit;
        std::size_t time_limit;
        bool attrs_only;
        std::unique_ptr<SearchFilter> filter;
        std::vector<std::string> attributes;
    };

    class SearchRequest : public Request {
      public:
        SearchRequest(int msg_id, SearchOptions options);
        SearchRequest(SearchRequest&&) = default;
        ~SearchRequest() override;

        static const std::uint8_t TAG = CONSTRUCTED(APPLICATION(3));

        static Result<SearchRequest, ParserError> try_from(int msg_id, ber::Tlv& tlv);

        ber::Tlv to_tlv() const override;
        std::uint8_t get_tag() const override;

        SearchOptions options;
    };

    struct SearchAttr {
        std::string type;
        std::vector<std::string> value;
    };

    class SearchResultEntry : public Response {
      public:
        SearchResultEntry(int request_id, std::string object_name, std::vector<SearchAttr> entry);
        ~SearchResultEntry() override;

        static const std::uint8_t TAG = CONSTRUCTED(APPLICATION(4));

        std::uint8_t get_tag() const override;

        std::string object_name;
        std::vector<SearchAttr> entry;

      private:
        ber::Tlv inner_tlv() const override;
    };

    class SearchResultDone : public Response {
      public:
        SearchResultDone(int request_id, LdapResult res);
        ~SearchResultDone() override;

        static const std::uint8_t TAG = CONSTRUCTED(APPLICATION(5));

        std::uint8_t get_tag() const override;

        LdapResult result;

      private:
        ber::Tlv inner_tlv() const override;
    };
}