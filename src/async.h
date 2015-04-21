//
// Created by RedDec on 19.04.15.
//

#ifndef IO_ASYNC_H
#define IO_ASYNC_H

#include "io.h"
#include "application.h"
#include <unordered_map>
#include <functional>
#include <sys/epoll.h>
#include <mutex>

namespace io {
    /**
     * Simple epoll wrapper with callbacks
     */
    struct Epoll : public Storage, public WithError {
        /**
         * Callback type.
         * - Epoll instance
         * - events
         * - descriptor
         */
        using Callback = std::function<void(Epoll &, uint32_t, int)>;

        /**
         * Move semantic
         */
        Epoll(Epoll &&that);

        /**
         * Move semantic
         */
        Epoll &operator=(Epoll &&that);

        /**
         * Initialize epoll instance with specified `flags` and events cache size. Check errors after it
         */
        Epoll(size_t cache_size = 100, int flags = 0);

        /**
         * Process events or unblock after `timeout` (in ms).
         * Returns count of processed events or less then 0 on error
         */
        int poll(uint32_t timeout = 0);

        /**
         * Add `callback` for descriptor `fd` with events `events_filter`.
         * Return state of operation
         */
        bool add(int fd, uint32_t events_filter, const Callback &callback);

        /**
         * Add callback for descriptor from class member. Just a wrapper for add(int, uint32_t, const Callback);
         */
        template<class T, class FunctionMember>
        inline bool add(int fd, uint32_t events_filter, FunctionMember member, T *obj) {
            return add(fd, events_filter,
                       std::bind(member, obj, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        /**
         * Remove callback
         */
        bool remove(int fd);

        /**
         * Change events filter for descriptor
         */
        bool update(int fd, uint32_t events_filter);

        /**
         * Change callback for desctiptor
         */
        bool update(int fd, Callback &callback);

        /**
         * Gets size of events cache (count of maximum events per poll)
         */
        inline size_t events_cache_size() const { return events_cache_.size(); }

        /**
         * Set maximum events per poll
         */
        void set_events_cache_size(size_t size) { events_cache_.resize(size); }

    private:
        std::vector<epoll_event> events_cache_;

        Epoll(const Epoll &) = delete;

        Epoll &operator=(const Epoll &) = delete;

        std::unordered_map<int, Callback> callbacks_;
    };


    /**
     * Abstract Epoll based async socket server
     */
    struct AsyncSocketServer {

        /**
         * Stop server, remove from epoll, close all clients and free allocated resources. Collection locked here
         */
        void stop();

        /**
         * Is server running
         */
        inline bool running() const { return running_; }

        /**
         * Get connection manager
         */
        inline io::ConnectionManager::Ptr server() { return server_; }

        /**
         * Stop server
         */
        virtual ~AsyncSocketServer();

    protected:
        /**
         * Create socket server and adds callbacks to EPOLL
         */
        AsyncSocketServer(io::Epoll &epoll, io::ConnectionManager::Ptr serv_con_);


        /**
         * Collection of clients sockets
         */
        std::unordered_map<int, io::FileStream::Ptr> clients_;

        /**
         * Lock collection for access from other threads
         */
        std::unique_lock<std::mutex> lock_collection();

        /**
         * Calls when server started and running
         */
        virtual void on_server_start() { }

        /**
         * Calls when server about to close
         */
        virtual void on_server_stopping() { }

        /**
         * Calls when all resources removed and server stopped
         */
        virtual void on_server_stopped() { }

        /**
         * Calls when new clients connected, but not added to collection yet
         */
        virtual void on_client_connected(io::FileStream::Ptr client) { }

        /**
         * Calls when client disconnected, but not remove from collection yet
         */
        virtual void on_client_disconnected(io::FileStream::Ptr client) { }

        /**
         * Calls when client sends some data
         */
        virtual void on_client_data_ready(io::FileStream::Ptr client) { }

        /**
         * Find client socket by descriptor or return null. Thread safe
         */
        inline io::FileStream::Ptr find_client_by_descriptor(int fd) const {
            std::lock_guard<std::mutex> guard(lock_);
            auto client = clients_.find(fd);
            if (client == clients_.end())return nullptr;
            return (*client).second;
        }

    private:
        /**
         * Collection lock
         */
        mutable std::mutex lock_;

        void on_server_event(io::Epoll &, uint32_t events, int fd); //Thread safe

        void on_client_event(io::Epoll &, uint32_t events, int client_fd);//Thread safe

        io::Epoll &poller_;

        io::ConnectionManager::Ptr server_;

        int server_fd_ = -1;

        bool running_ = false;
    };


    /**
     * Universal IO interface for Epoll events: IN/ERR/HUP/RDHUP
     */
    struct AbstractAsyncFile {

        /**
         * Get underlying file stream
         */
        inline io::FileStream &file() { return file_d; }

        /**
         * Close file
         */
        void stop();

        /**
         * Close file
         */
        virtual  ~AbstractAsyncFile();

    protected:
        /**
         * Initialize resources and register for epoll events if all is ok
         */
        AbstractAsyncFile(int fd, io::Epoll &epoll, uint32_t custom_events = 0);

        /**
         * When EPOLLIN events
         */
        virtual void on_data(io::FileStream &file) { }

        /**
         * On HUP, RDHUP, ERR. Closes stream after it.
         */
        virtual void on_close(io::FileStream &file) { }

        /**
         * Before file will be closed on stop function
         */
        virtual void on_stop(io::FileStream &file) { }

        /**
         * After adding in epoll events
         */
        virtual void on_start(io::FileStream &file) { }

        /**
         * On unrecognized event
         */
        virtual void on_event(io::FileStream &file) { }

    private:
        io::FileStream file_d;

        void process_event(io::Epoll &ep, uint32_t events, int fd);
    };
}

#endif //IO_ASYNC_H
