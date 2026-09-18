# LDAP Server challenge
This is repository contains my solution to a programming challenge, whose goal was to create a simple LDAPv2 server.
It is my first attempt at writing something non-trivial in C++, so expect bugs, horrible coding practices and outright sorcery.

## Goal
I am to make a simple thread-per-connection LDAPv2 server using blocking IO, listening on a user specified TCP port, able to accept
connections from both IPv4 and IPv6. It should be able to respond to simple queries made by `ldapsearch`. The server should stop quickly when SIGINT is sent to it.   
I would also love to learn a bit of somewhat modern C++, along with OS related concepts like signals, threads, and network IO.

## Rules
- C++14 (will maybe change that to a newer version sometime)
- using only POSIX compliant APIs (no epoll, signalfd, ...)
- no third-party dependencies
- no AI generated code, almost no AI usage in general
- try to handle most edge cases and errors

## Notes
Currently, only the equality match search filter is supported. Any unsupported TLV will cause the program to abort. This is temporary until I decide whether to implement it or not in the future.

## Resources
- [LDAPv2 protocol (RFC 1777)](https://datatracker.ietf.org/doc/html/rfc1777)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [ASN.1 BER Encoding Rules](https://www.oss.com/asn1/resources/asn1-made-simple/asn1-quick-reference/basic-encoding-rules.html)