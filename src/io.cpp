//
// Created by Red Dec on 19.04.15.
//

#include "io.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <cstring>

namespace io {
    void Storage::close() {
        if (descriptor_ >= 0) {
            ::close(descriptor_);
            descriptor_ = -1;
        }
    }

    Storage::~Storage() {
        if (auto_close_)close();
    }

    FileReadBuffer::FileReadBuffer(int d, std::size_t chunk_size)
            : chunk_(chunk_size), buffer_(chunk_size) {
        descriptor_ = d;
    }

    std::streambuf::int_type FileReadBuffer::underflow() {
        if (gptr() < egptr())  // buffer not exhausted
            return traits_type::to_int_type(*gptr());
        if (!has_valid_descriptor()) traits_type::eof();
        ssize_t n = read(descriptor_, buffer_.data(), chunk_);
        if (n <= 0) return traits_type::eof();
        char *base = &buffer_.front();
        char *start = base;
        setg(base, start, start + n);
        return traits_type::to_int_type(*gptr());
    }

    FileWriteBuffer::FileWriteBuffer(int d, std::size_t chunk_size)
            : chunk_(chunk_size), buffer_(chunk_size) {
        descriptor_ = d;
    }

    std::streambuf::int_type FileWriteBuffer::overflow(
            std::streambuf::int_type ch) {
        if (!has_valid_descriptor()) traits_type::eof();
        if (ch == traits_type::eof()) {
            sync();
            return traits_type::eof();
        }
        buffer_[count_++] = ch;
        if (count_ >= chunk_) return sync();
        return ch;
    }

    int FileWriteBuffer::sync() {
        ssize_t part = 1;
        size_t count = 0;
        while (count < count_) {
            part = write(descriptor_, &buffer_[count], count_ - count);
            if (part <= 0) return traits_type::eof();
            count += static_cast<size_t>(part);
        }
        count_ = 0;
        return 0;
    }

    AddressInfo::AddressInfo(const std::string &hostDomainOrIp, const std::string &serviceOrPort) {
        if (getaddrinfo(hostDomainOrIp.c_str(), serviceOrPort.c_str(), nullptr, &info_) != 0) set_error();
    }


    AddressInfo::AddressInfo(const std::string &hostDomainOrIp, uint16_t port) {
        AddressInfo(hostDomainOrIp, std::to_string(port));
    }


    AddressInfo::~AddressInfo() {
        if (info_ != nullptr) {
            freeaddrinfo(info_);
        }
    }


    ConnectionManager::~ConnectionManager() {

    }


    AbstractSocketManager::~AbstractSocketManager() {
        close();
    }

    int AbstractSocketManager::next_descriptor() {
        if (!is_active()) return -1;
        int client = ::accept(descriptor_, nullptr, nullptr);
        if (client < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) set_error(); //Do not print error in NON-BLOCKING mode
            return -1;
        }
        return client;
    }

    bool AbstractSocketManager::set_accept_timeout(uint64_t milliseconds) {
        if (!is_active()) return false;
        struct timeval timeout;
        timeout.tv_sec = milliseconds / 1000;
        timeout.tv_usec = (milliseconds % 1000) * 1000;
        if (setsockopt(descriptor_, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout,
                       sizeof(timeout)) < 0) {
            set_error();
            return false;
        }
        return true;

    }

    TcpServerManager::TcpServerManager(const std::string &service, const std::string &bind_host, int backlog) {
        descriptor_ = socket(AF_INET6, SOCK_STREAM, 0);
        if (!has_valid_descriptor()) return;
        AddressInfo info(bind_host, service);
        if (info.has_error()) {
            set_error();
            close();
            return;
        }
        if (bind(descriptor_, (*info)->ai_addr, (*info)->ai_addrlen) < 0) {
            set_error();
            close();
            return;
        }
        int opt = 1;
        if (setsockopt(descriptor_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            set_error();
            close();
            return;
        }
        if (listen(descriptor_, backlog) < 0) {
            set_error();
            close();
            return;
        };
    }

    UnixServerManager::UnixServerManager(const std::string &path, int backlog, uint32_t mode) :
            path_(path) {

        struct sockaddr_un address;
        descriptor_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (!has_valid_descriptor()) {
            set_error();
            return;
        }
        memset(&address, 0, sizeof(struct sockaddr_un));
        address.sun_family = AF_UNIX;
        std::memmove(address.sun_path, path.c_str(), std::min(path.size() + 1, sizeof(address.sun_path)));
        if (bind(descriptor_, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) < 0) {
            set_error();
            close();
            return;
        }

        if (listen(descriptor_, backlog) < 0) {
            set_error();
            close();
            return;
        }

        if (chmod(path.c_str(), mode) < 0) {
            set_error();
            close();
            return;
        }
    }

    std::shared_ptr<TcpServerManager> TcpServerManager::create(const std::string &service, std::string const &bind_host,
                                                               int backlog) {
        return std::make_shared<TcpServerManager>(service, bind_host, backlog);
    }

    std::shared_ptr<UnixServerManager> UnixServerManager::create(const std::string &path, int backlog, uint32_t mode) {
        return std::make_shared<UnixServerManager>(path, backlog, mode);
    }

    UnixServerManager::~UnixServerManager() {
        remove(path_.c_str());
    }


    FileStream::FileStream(int fd) noexcept :
            input_buffer(fd),
            output_buffer(fd),
            input_(&input_buffer),
            output_(&output_buffer) {
        descriptor_ = fd;
        set_auto_close(true);
    }

    FileStream::Ptr FileStream::create(int fd) {
        return std::make_shared<FileStream>(fd);
    }

    const std::string &version() {
        static std::string version_ = BUILD_VERSION;
        return version_;
    }
}