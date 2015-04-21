#ifndef IO_IO_H
#define IO_IO_H

#include <streambuf>
#include <vector>
#include <memory>
#include <netdb.h>
#include <cstring>
#include <iostream>

namespace io {
    const std::string &version();

    template<size_t sz = 1024>
    static inline std::string ngetline(std::istream &in) {
        char text[sz];
        size_t ln;
        memset(text, 0, sz);
        in.getline(text, sz - 1);
        ln = strnlen(text, sz);
        if (ln > 0 && text[ln - 1] == '\r')--ln;
        return std::string(text, ln);
    }

    struct Storage {
        Storage();

        explicit Storage(int fd, bool close_at_end = false);

        Storage(Storage &&that);

        Storage &operator=(Storage &&);

        /**
         * Get active file descriptor
         */
        inline int descriptor() const { return descriptor_; };

        /**
         * Close descriptor
         */
        virtual void close();

        /**
         * Is descriptor valid
         */
        inline bool has_valid_descriptor() const { return descriptor_ >= 0; }

        /**
         * Does descriptor close on descriptor
         */
        inline void set_auto_close(bool enable) { auto_close_ = enable; }

        inline bool auto_close() const { return auto_close_; }

        virtual ~Storage();

    protected:
        int descriptor_ = -1;
    private:
        Storage(const Storage &) = delete;

        Storage &operator=(const Storage &) = delete;

        bool auto_close_ = false;

    };

    struct WithError {

        /**
         * Does error exists
         */
        inline bool has_error() const { return error_; }

        /**
         * Get error message or empty string
         */
        inline const std::string &error_message() const {
            if (error_ && error_message_.empty()) error_message_ = strerror(error_code_);
            return error_message_;
        }

        /**
         * Get error code or 0 if nothing
         */
        inline int error_code() const { return error_code_; }

    protected:
        /**
         * Set error from errno
         */
        inline void set_error() {
            error_ = true;
            error_code_ = errno;
        }

        /**
         * Set custom error with non empty message. If message empty, '<unknown error>' will be set
         */
        inline void set_error(int err_code, const std::string &message) {
            error_message_ = message.empty() ? "<unknown error>" : message;
            error_code_ = err_code;
            error_ = true;
        }

    private:
        bool error_ = false;
        int error_code_ = 0;
        mutable std::string error_message_;
    };

/**
 * Reader from file descriptor. If descriptor less then 0 or `read` returns less or equal 0, EOF will be set.
 * This class doesn't close descriptor automatically
 */
    struct FileReadBuffer : public std::streambuf, public Storage {

        /**
         * Initialize internal buffer for descriptor `d`. Single portion of incoming data has size `chunk_size`
         */
        explicit FileReadBuffer(int d, std::size_t chunk_size = 8192);

    private:
        int_type underflow();

        FileReadBuffer(const FileReadBuffer &) = delete;

        FileReadBuffer &operator=(const FileReadBuffer &) = delete;

        std::size_t chunk_;
        std::vector<char> buffer_;
    };

/**
 *  Writer to file descriptor. If descriptor less then 0 or `write` returns less or equal 0, EOF will be set.
 *  his class doesn't close descriptor automatically
 */
    struct FileWriteBuffer : public std::streambuf, public Storage {
    public:

        /**
         * Initialize internal buffer for descriptor `d`. Single portion of outgoing data has size`chunk_size`
         */
        explicit FileWriteBuffer(int d, std::size_t chunk_size = 8192);

    private:
        FileWriteBuffer(const FileWriteBuffer &) = delete;

        FileWriteBuffer &operator=(const FileWriteBuffer &) = delete;

        int_type overflow(int_type ch);

        int sync();

        std::size_t chunk_, count_ = 0;
        std::vector<char> buffer_;
    };

/**
 * Address information
 */
    struct AddressInfo : public WithError {

        /**
         * Retrieve address information by host/domain/ip and service/port
         */
        AddressInfo(const std::string &hostDomainOrIp, const std::string &serviceOrPort);

        /**
         * Retrieve address information by host/domain/ip and port
         */
        AddressInfo(const std::string &hostDomainOrIp, uint16_t port);

        /**
         * Returns address info or null if error occurred
         */
        inline const addrinfo *data() const { return info_; }

        /**
         * Returns address info or null if error occurred
         */
        inline const addrinfo *operator*() const { return info_; }


        /**
         * Free allocated memory
         */
        ~AddressInfo();

    private:
        addrinfo *info_ = nullptr;
    };

/**
* Connection manager for new requests. For example via Unix or Tcp socket
*/
    struct ConnectionManager : public Storage, public WithError {
        /**
        * Returns current sate of connection manager
        */
        virtual bool is_active() = 0;

        /**
        * Wait for new file descriptor. Returns -1 on error
        */
        virtual int next_descriptor() = 0;

        virtual ~ConnectionManager();

        typedef std::shared_ptr<ConnectionManager> Ptr;
    };

/**
* Abstract socket functional. ::close and ::accept
*/
    struct AbstractSocketManager : public ConnectionManager {

        /**
        * Accept new client or returns -1 on error
        */
        virtual int next_descriptor() override;

        /**
        * Calls stop()
        */
        virtual  ~AbstractSocketManager();

        /**
        * Returns true if descriptor has been created and wasn't closed yet
        */
        virtual inline bool is_active() override {
            return has_valid_descriptor();//FIXME: Stub
        }

        /**
         * Set accept timeout or -1 (uint64_t max) for infinity loop.
         * Return true if operation done. Otherwise prints error and returns false;
         */
        bool set_accept_timeout(uint64_t milliseconds = -1);

        inline uint64_t accept_timeout() const { return timeout_; }

    protected:
        uint64_t timeout_ = -1;
    };

/**
* Simple blocking TCP IPv6 server
*/
    struct TcpServerManager : public AbstractSocketManager {

        /**
        * Create server socket, bind it to `bind_host` and port `service` with listen queue `backlog`
        */
        TcpServerManager(const std::string &service, const std::string &bind_host = "::", int backlog = 100);

        static std::shared_ptr<TcpServerManager> create(const std::string &service, const std::string &bind_host = "::",
                                                        int backlog = 100);

    };


/**
* Simple blocking UNIX socket server
*/
    struct UnixServerManager : public AbstractSocketManager {

        /**
        * Create UNIX server socket, bind it to `path` with listen queue `backlog`
        */
        UnixServerManager(const std::string &path, int backlog = 100, uint32_t mode = 0777);

        inline const std::string &path() const { return path_; }

        static std::shared_ptr<UnixServerManager> create(const std::string &path, int backlog = 100,
                                                         uint32_t mode = 0777);


        virtual ~UnixServerManager();

    private:
        std::string path_;
    };

    /**
     * IO stream based on file descriptor
     */
    struct FileStream : public Storage {
        /**
         * Shared pointer
         */
        typedef std::shared_ptr<FileStream> Ptr;

        /**
         * Initialize stream
         */
        FileStream(int fd) noexcept;

        /**
         * Get input stream
         */
        inline std::istream &input() noexcept { return input_; }

        /**
         * Get output stream
         */
        inline std::ostream &output() noexcept { return output_; }

        /**
         * Initialize new instance of FileStream and wrap it to shared pointer
         */
        static Ptr create(int fd);

    private:
        FileReadBuffer input_buffer;
        FileWriteBuffer output_buffer;
        std::istream input_;
        std::ostream output_;
    };
}
#endif //IO_IO_H