//
// Created by RedDec on 20.04.15.
//

#include "experimental.h"

namespace io {

    Publisher::Publisher(io::ConnectionManager::Ptr ptr) : manager_(ptr) {
        if (ptr->has_valid_descriptor()) {
            if (!epoll_.add(ptr->descriptor(), EPOLLIN, &Publisher::on_client, this))
                set_error();
        }
        if (ready()) {
            thread_ = std::thread(std::bind(&Publisher::run, this));
        }
    }

    Publisher::~Publisher() {
        stop();
        thread_.join();
    }

    void Publisher::clean() {
        for (auto &kv:clients_) epoll_.remove(kv.first);
        clients_.clear();
        epoll_.remove(manager_->descriptor());
    }

    void Publisher::run() {
        int count = 0;
        while (count >= 0 && ready()) {
            count = epoll_.poll(5000);
        }
    }

    void Publisher::stop() {
        clean();
        stop_ = true;
        epoll_.close();
    }

    void Publisher::on_client(io::Epoll &epoll, uint32_t event, int fd) {
        std::unique_lock<std::mutex> lock(lock_);
        if (event & EPOLLIN) {
            int client = manager_->next_descriptor();
            epoll.add(client, EPOLLHUP | EPOLLRDHUP | EPOLLERR, &Publisher::on_client_disconnect, this);
            clients_[client] = io::FileStream::create(client);
        } else {
            //Error on accept - this is fatal
            set_error();
            stop();
        }
    }

    void Publisher::on_client_disconnect(io::Epoll &epoll, uint32_t event, int fd) {
        std::unique_lock<std::mutex> lock(lock_);
        epoll.remove(fd);
        clients_.erase(fd);
    }
}