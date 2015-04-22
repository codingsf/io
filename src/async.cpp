//
// Created by Red Dec on 19.04.15.
//

#include "async.h"

namespace io {
    Epoll::Epoll(size_t cache_size, int flags) : events_cache_(cache_size) {
        descriptor_ = epoll_create1(flags);
        if (!has_valid_descriptor()) set_error();
        set_auto_close(true);
    }

    Epoll::Epoll(Epoll &&that) {
        descriptor_ = that.descriptor_;
        events_cache_ = that.events_cache_;
        callbacks_ = that.callbacks_;
        that.descriptor_ = -1;
        that.events_cache_.clear();
        that.callbacks_.clear();
    }

    Epoll &Epoll::operator=(Epoll &&that) {
        std::swap(descriptor_, that.descriptor_);
        std::swap(events_cache_, that.events_cache_);
        std::swap(callbacks_, that.callbacks_);
        return *this;
    }

    bool Epoll::add(int fd, uint32_t events_filter, const Epoll::Callback &callback) {
        if (!has_valid_descriptor() || fd < 0)return false;
        epoll_event event;
        event.events = events_filter;
        event.data.fd = fd;
        bool ok = epoll_ctl(descriptor_, EPOLL_CTL_ADD, fd, &event) == 0;
        if (ok) callbacks_[fd] = callback;
        else set_error();
        return ok;
    }

    bool Epoll::remove(int fd) {
        if (fd < 0) return false;
        auto callback = callbacks_.find(fd);
        if (callback == callbacks_.end())return false;
        callbacks_.erase(callback);
        if (!has_valid_descriptor()) return false;
        bool ok = epoll_ctl(descriptor_, EPOLL_CTL_DEL, fd, nullptr) == 0;
        if (!ok) set_error();
        return ok;
    }

    bool Epoll::update(int fd, uint32_t events_filter) {
        if (fd < 0 || !has_valid_descriptor())return false;
        epoll_event event;
        event.events = events_filter;
        event.data.fd = fd;
        bool ok = epoll_ctl(descriptor_, EPOLL_CTL_MOD, fd, &event) == 0;
        if (!ok) set_error();
        return ok;
    }

    bool Epoll::update(int fd, Epoll::Callback &callback) {
        if (!has_valid_descriptor() || fd < 0)return false;
        auto iter = callbacks_.find(fd);
        if (iter == callbacks_.end())return false;
        (*iter).second = callback;
        return true;
    }

    int Epoll::poll(int timeout) {
        if (!has_valid_descriptor()) {
            set_error(-1, "Invalid descriptor");
            return -1;
        }
        int res = epoll_wait(descriptor_, events_cache_.data(), events_cache_.size(), timeout);
        if (res >= 0)
            for (int i = 0; i < res; ++i) {
                callbacks_[events_cache_[i].data.fd](*this, events_cache_[i].events, events_cache_[i].data.fd);
            }
        else
            set_error();
        return res;
    }

    AsyncSocketServer::AsyncSocketServer(io::Epoll &epoll, io::ConnectionManager::Ptr serv_con_) : poller_(epoll),
                                                                                                   server_(serv_con_),
                                                                                                   server_fd_(
                                                                                                           serv_con_->descriptor()) {
        running_ = poller_.has_valid_descriptor() &&
                   serv_con_->has_valid_descriptor() &&
                   poller_.add(server_fd_, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP,
                               &AsyncSocketServer::on_server_event,
                               this);
        if (running_) on_server_start();
    }

    void AsyncSocketServer::stop() {
        if (running_) {
            on_server_stopping();
            {
                auto lock = lock_collection();
                for (auto &it:clients_)
                    it.second->close();
                clients_.clear();
                poller_.remove(server_fd_);
            }
            running_ = false;
            on_server_stopped();
        }
    }

    AsyncSocketServer::~AsyncSocketServer() {
        stop();
    }

    void AsyncSocketServer::on_server_event(io::Epoll &, uint32_t events, int fd) {
        if (events & EPOLLERR) {
            stop();
        } else if (events & EPOLLIN) {
            int client_fd = server_->next_descriptor();
            if (client_fd < 0) stop();
            else {
                auto client = io::FileStream::create(client_fd);
                on_client_connected(client);
                {
                    std::lock_guard<std::mutex> guard(lock_);
                    clients_[client_fd] = client;
                }
            }
        }
    }

    void AsyncSocketServer::on_client_event(io::Epoll &, uint32_t events, int client_fd) {
        auto client = find_client_by_descriptor(client_fd);
        if (!client) return; //Already removed
        if (events & (EPOLLERR)) {
            client->close();
        } else if (events & (EPOLLHUP | EPOLLRDHUP)) {
            on_client_disconnected(client);
            client->close();
            {
                std::lock_guard<std::mutex> guard(lock_);
                clients_.erase(client_fd);
                poller_.remove(client_fd);
            }
        } else if (events & (EPOLLIN)) {
            on_client_data_ready(client);
        } else {
            //STUB for future events
        }
    }

    std::unique_lock<std::mutex> AsyncSocketServer::lock_collection() {
        return std::unique_lock<std::mutex>(lock_);
    }


    AbstractAsyncFile::AbstractAsyncFile(int fd, io::Epoll &epoll, uint32_t custom_events) : file_d(fd) {
        if (file_d.has_valid_descriptor() && epoll.has_valid_descriptor() &&
            epoll.add(fd, EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP | custom_events, &AbstractAsyncFile::process_event,
                      this)) {
            on_start(file_d);
        }
    }

    void AbstractAsyncFile::stop() {
        if (file_d.has_valid_descriptor()) {
            on_stop(file_d);
            file_d.close();
        }
    }

    AbstractAsyncFile::~AbstractAsyncFile() {
        stop();
    }

    void AbstractAsyncFile::process_event(io::Epoll &ep, uint32_t events, int fd) {
        if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
            on_close(file_d);
            ep.remove(fd);
            file_d.close();
        } else if (events & EPOLLIN) {
            on_data(file_d);
        } else {
            on_event(file_d, events);
        }
    }
}