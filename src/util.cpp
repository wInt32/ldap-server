#include <cstddef>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <unistd.h>
#include <vector>
#include <poll.h>
#include <sstream>

#include "util.hpp"

std::string char_to_hex(unsigned char ch) {
    std::string s{"0x"};
    unsigned char nibble_1 = ch & 0x0F;
    unsigned char nibble_2 = (ch & 0xF0) >> 4;

    nibble_1 += (nibble_1 <= 9 ? '0' : 'a'-10);
    nibble_2 += (nibble_2 <= 9 ? '0' : 'a'-10);

    s.push_back(nibble_2);
    s.push_back(nibble_1);
    return s;
}

void print_bytes(std::basic_ostream<char> &o, const std::string& v) {
    o << "Received data: [";
    for (std::size_t i = 0; i < v.size(); i++) {
        o << char_to_hex(v[i]);
        if (i < v.size()-1) o << ", ";
    }
    o << "]" << std::endl;
}

void print_bytes(std::basic_ostream<char> &o, const std::vector<std::uint8_t> &v) {
    o << "Received data: [";
    for (std::size_t i = 0; i < v.size(); i++) {
        o << char_to_hex(v[i]);
        if (i < v.size()-1) o << ", ";
    }
    o << "]" << std::endl;
}

ShutdownPipe::ShutdownPipe() {
    int res = ::pipe(this->fd);
    if (res < 0) {
        ::perror("pipe()");
        std::exit(1);
    }
}

int ShutdownPipe::rfd() {
    return this->fd[0];
}

int ShutdownPipe::wfd() {
    return this->fd[1];
}

char ShutdownPipe::read() {
    char c;
    do_read: int res = ::read(this->rfd(), &c, 1);
    if (res < 0) {
        if (errno == EAGAIN || errno == EINTR) goto do_read;
        perror("reading from shutdown pipe");
    }
    return c;
}

void ShutdownPipe::write(char c) {
    do_write: int res = ::write(this->wfd(), &c, 1);
    if (res < 0) {
        if (errno == EAGAIN || errno == EINTR) goto do_write;
        perror("writing to shutdown pipe");
    }
}

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
constexpr bool is_host_be() {
    return true;
}
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
constexpr bool is_host_be() {
    return false;
}
#else
bool is_host_be() {
    std::uint16_t x = 0x01;
    const void *p = static_cast<const void *>(&x);
    const std::uint8_t *c = static_cast<const std::uint8_t *>(p);
    return *c == 0x00;
}
#endif

std::uint32_t bswap_if_needed(std::uint32_t a) {
    if (is_host_be()) return a;

    int res = 0;
    res |= (a & 0x000000ff) << 24;
    res |= (a & 0x0000ff00) << 8;
    res |= (a & 0x00ff0000) >> 8;
    res |= (a & 0xff000000) >> 24;

    return res;
}

Stream::~Stream() {}

SocketStream::SocketStream() : fd{-1}, rbuf{0} {}

SocketStream::SocketStream(int fd) : fd{fd}, rbuf{0} {}

SocketStream::SocketStream(SocketStream&& other) : fd{other.fd}, rbuf{0} {
    other.fd = -1;
}

int shutdown_aware_read(int fd, void* buf, std::size_t n) {
    const std::size_t FD = 0;
    const std::size_t SHUTDOWN = 1;

    ::pollfd events[2] = {};
    events[FD] = {fd, POLLIN, 0};
    events[SHUTDOWN] = {shutdown_pipe.rfd(), POLLIN, 0};

    const std::size_t events_len = sizeof(events)/sizeof(events[0]);

    do_poll: int ev_count = ::poll(events, events_len, -1);
    if (ev_count < 0) {
        if (errno == EAGAIN || errno == EINTR) goto do_poll;
        perror("poll()");
        return -1;
    }

    short int fd_events = events[FD].revents;
    short int shutdown_events = events[SHUTDOWN].revents;

    if (fd_events != 0) {
        return ::read(fd, buf, n);
    }
    if (shutdown_events != 0) {
        return 0;
    }

    // unreachable
    std::abort();
}

Result<std::vector<std::uint8_t>, IOError> SocketStream::read_exact(std::size_t n) {
    std::vector<std::uint8_t> content{};
    content.resize(n);
    Optional<IOError> e = this->read_exact_raw(content.data(), n);
    if (e.has_value) return e.value;
    return {content};
}

Optional<IOError> SocketStream::read_exact_raw(void *dst, std::size_t n) {
    std::size_t bytes_read = 0;

    while (true) {
        // first use data from the read buffer
        std::size_t available = std::min(this->rbuf_whead-this->rbuf_rhead, n);

        if (available > 0) {
            std::memcpy(dst, this->rbuf+this->rbuf_rhead, available);
            this->rbuf_rhead += available;
            bytes_read += available;
            if (bytes_read >= n) break;
        }

        // if we need more data, read from network
        this->rbuf_whead = 0;
        this->rbuf_rhead = 0;
        int res = shutdown_aware_read(this->fd, this->rbuf+this->rbuf_whead, sizeof(this->rbuf));
        //std::cout << "Read: " << res << std::endl; // TODO: remove logging
        if (res == 0) return IOError{this->fd, ENOTCONN};
        if (res < 0) return IOError{this->fd, errno};
        this->rbuf_whead += res;
    }

    return {};
}

Result<std::uint8_t, IOError> SocketStream::peek_byte() {
    std::uint8_t byte;
    Optional<IOError> e = this->read_exact_raw(&byte, 1);
    if (e.has_value) return {e.value};
    this->rbuf_rhead--;
    return {byte};
}

Result<std::uint8_t, IOError> SocketStream::read_byte() {
    std::uint8_t byte;
    Optional<IOError> e = this->read_exact_raw(&byte, 1);
    if (e.has_value) return {e.value};
    return {byte};
}

Optional<IOError> SocketStream::write(const std::vector<std::uint8_t>& bytes) {
    do_write: int res = ::write(this->fd, bytes.data(), bytes.size());
    if (res < 0) {
        if (errno == EAGAIN || errno == EINTR) goto do_write;
        return IOError{this->fd, errno};
    }
    return {}; 
}

void SocketStream::reset() {
    this->fd = -1;
}

SocketStream::~SocketStream() {}

void SocketStream::operator=(SocketStream&& other) {
    this->fd = other.fd;
    std::memcpy(this->rbuf, other.rbuf, sizeof(this->rbuf));
    this->rbuf_rhead = other.rbuf_rhead;
    this->rbuf_whead = other.rbuf_whead;
}

ShutdownPipe shutdown_pipe{};

std::string IOError::str() {
    std::stringstream ss{};
    ss << "error on fd/" << this->fd << " : ";
    ss << std::strerror(this->code) << '\n';
    return ss.str();
}