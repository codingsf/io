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

    bool Epoll::add(int fd, uint32_t events_filter, const Epoll::Callback &callback) {
        if (!has_valid_descriptor() || fd < 0)return false;
        epoll_event event;
        event.events = events_filter;
        event.data.fd = fd;
        bool ok = epoll_ctl(descriptor_, EPOLL_CTL_ADD, fd, &event) == 0;
        if (ok) callbacks_[fd] = callback;
        return ok;
    }

    bool Epoll::remove(int fd) {
        if (fd < 0)return false;
        auto callback = callbacks_.find(fd);
        if (callback == callbacks_.end())return false;
        callbacks_.erase(callback);
        return has_valid_descriptor() && epoll_ctl(descriptor_, EPOLL_CTL_DEL, fd, nullptr) == 0;
    }

    bool Epoll::update(int fd, uint32_t events_filter) {
        if (fd < 0 || !has_valid_descriptor())return false;
        epoll_event event;
        event.events = events_filter;
        event.data.fd = fd;
        return epoll_ctl(descriptor_, EPOLL_CTL_MOD, fd, &event) == 0;
    }

    bool Epoll::update(int fd, Epoll::Callback &callback) {
        if (!has_valid_descriptor() || fd < 0)return false;
        auto iter = callbacks_.find(fd);
        if (iter == callbacks_.end())return false;
        (*iter).second = callback;
        return true;
    }

    int Epoll::poll(uint32_t timeout) {
        if (!has_valid_descriptor() || has_error())return 0;
        int res = epoll_wait(descriptor_, events_cache_.data(), events_cache_.size(), timeout);
        if (res >= 0)
            for (int i = 0; i < res; ++i) {
                callbacks_[events_cache_[i].data.fd](*this, events_cache_[i].events, events_cache_[i].data.fd);
            }
        else
            set_error();
        return res;
    }
}