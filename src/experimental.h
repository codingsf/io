//
// Created by RedDec on 20.04.15.
//

#ifndef IO_EXPERIMENTAL_H
#define IO_EXPERIMENTAL_H

#include "io.h"
#include "async.h"
#include <mutex>
#include <thread>

namespace io {

    struct Publisher : public io::WithError {
        Publisher(io::ConnectionManager::Ptr ptr);

        template<class T>
        void write_line(const T &obj) {
            std::unique_lock<std::mutex> lock(lock_);
            for (auto &kv: clients_) {
                kv.second->output() << obj << std::endl;
            }
        }

        inline bool ready() const {
            return !stop_ &&
                   manager_->is_active() &&
                   manager_->has_valid_descriptor() &&
                   epoll_.has_valid_descriptor() &&
                   !epoll_.has_error() &&
                   !manager_->has_error();
        }

        ~Publisher();

    protected:

        void clean(); //Remove clients and server from epoll

        void run(); // Poll events

        void stop(); // Clean and close epoll

        void on_client(io::Epoll &epoll, uint32_t event, int fd);

        void on_client_disconnect(io::Epoll &epoll, uint32_t event, int fd);


    private:
        bool stop_ = false;
        std::thread thread_;
        std::mutex lock_;
        io::ConnectionManager::Ptr manager_;
        io::Epoll epoll_;
        std::unordered_map<int, io::FileStream::Ptr> clients_;
        std::function<void()> idle_;
    };
}
#endif //IO_EXPERIMENTAL_H
