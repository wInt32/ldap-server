#pragma once
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <cstring>
#include <type_traits>
#include <vector>
#include <ostream>
#include <memory>

template<typename T>
class Optional {
  public:
    Optional() {
        this->has_value = false;
    }

    Optional(T val) : value{std::move(val)} {
        this->has_value = true;
    }
    Optional(const Optional& other) {
        this->has_value = other.has_value;
        if (other.has_value) value = other.value;
    }
    Optional(Optional&& other) : value{std::move(other.value)} {
        other.has_value = false;
    }
    ~Optional() {}

    Optional swap(Optional& other) {
        Optional<T> a{};
        a.has_value = this->has_value;
        std::memcpy(a.buf, this->buf, sizeof(T));
        this->has_value = other.has_value;
        std::memcpy(this->buf, other.buf, sizeof(T));
        return a;
    }
    
    std::shared_ptr<T> into_shared() {
        if (!this->has_value) return nullptr;
        return std::make_shared<T>(std::move(this->value));
    }
    std::unique_ptr<T> into_unique();

    bool has_value;
    union {
        T value;
        std::uint8_t buf[sizeof(T)];
    };
};

template<typename T, typename E>
class Result {
  public:
    Result() : ok{false}, error{} {}
    Result(T value) : value{std::move(value)}, ok{true} {}
    Result(E error) : error{std::move(error)}, ok{false} {}
    
    Result(const Result& other) : ok{other.ok} {
        if (other.ok) {
            this->value = other.value;
        } else {
            this->error = other.error;
        }
    }
    Result(Result&& other) : ok{other.ok} {
        if (other.ok) {
            this->value = std::move(other.value);
        } else {
            this->error = std::move(other.error);
        }
    }
    ~Result() {
        if (this->ok && std::is_destructible<T>()) value.~T();
    }

    union {
        T value;
        E error;
    };
    bool ok;
};

class IOError {
  public:
    std::string str();

    int fd;
    int code;
};

class ShutdownPipe {
  public:
    ShutdownPipe();
    void write(char c);
    void write_n(int n);
    char read();
    int rfd();
    int wfd();
  private:
    int fd[2];
};

extern ShutdownPipe shutdown_pipe;

class Stream {
  public:
    virtual ~Stream();

    virtual Result<std::vector<std::uint8_t>, IOError> read_exact(std::size_t n) = 0;
    virtual Optional<IOError> read_exact_raw(void *dst, std::size_t n) = 0;
    virtual Result<std::uint8_t, IOError> read_byte() = 0;
    virtual Result<std::uint8_t, IOError> peek_byte() = 0;
    virtual Optional<IOError> write(const std::vector<std::uint8_t>& bytes) = 0;
};

class SocketStream : public Stream {
  public:
    SocketStream();
    SocketStream(int fd);
    SocketStream(SocketStream&&);
    SocketStream(const SocketStream&) = delete;
    void operator=(SocketStream&&);
    ~SocketStream() override;

    virtual Result<std::vector<std::uint8_t>, IOError> read_exact(std::size_t n) override;
    virtual Optional<IOError> read_exact_raw(void *dst, std::size_t n) override;
    virtual Result<std::uint8_t, IOError> read_byte() override;
    virtual Result<std::uint8_t, IOError> peek_byte() override;
    virtual Optional<IOError> write(const std::vector<std::uint8_t>& bytes) override;

    void reset();
    int fd;

  private:
    char rbuf[256];
    std::size_t rbuf_whead = 0;
    std::size_t rbuf_rhead = 0;
};

#define TODO() do { \
    extern int printf(const char*, ...); \
    ::printf("%s:%d: Not yet implemented!\n", __FILE__, __LINE__); \
    std::abort(); \
} while (0)


std::string char_to_hex(unsigned char ch);
void print_bytes(std::basic_ostream<char> &o, const std::vector<std::uint8_t> &v);
void print_bytes(std::basic_ostream<char> &o, std::string s);
std::uint32_t bswap_if_needed(std::uint32_t a);

int shutdown_aware_read(int fd, void* dst, std::size_t n);