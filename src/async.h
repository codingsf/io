//
// Created by RedDec on 19.04.15.
//

#ifndef IO_ASYNC_H
#define IO_ASYNC_H

#include "io.h"
#include <unordered_map>
#include <functional>
#include <sys/epoll.h>

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
}
#endif //IO_ASYNC_H
