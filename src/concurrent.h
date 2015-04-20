//
// Created by Red Dec on 19.04.15.
//

#ifndef IO_CONCURRENT_H
#define IO_CONCURRENT_H

#include <mutex>
#include <queue>
#include <condition_variable>

namespace io {
/**
* Concurrent blocking queue
*/
    template<class T, class QUEUE = std::queue<T> >
    class BlockingQueue {

    public:
        enum : uint64_t {
            Infinity = 0,
            Millisecond = 1,
            Second = 1000 * Millisecond,
            Minute = 60 * Second,
            Hour = 60 * Minute,
            Day = 24 * Hour
        };

        /**
     * Push data to queue and notify threads
     */
        inline bool push(const T &var) {
            std::unique_lock<std::mutex> lock(mutex);
            if (is_finished())return false;
            queue_.push(var);
            monitor.notify_one();
            return true;
        }

        /**
     * Get (and pop) one element from queue. If queue is empty, threads will be waiting until queue is closed or
     * data pushed.
     * Returns false if queue is closed
     */
        inline bool pop(T &var, uint64_t timeout = Infinity) {
            while (true) {
                std::unique_lock<std::mutex> lock(mutex);
                if (finalized) break;
                if (!queue_.empty()) {
                    var = queue_.front();
                    queue_.pop();
                    return true;
                }
                if (timeout == Infinity) monitor.wait(lock);
                else {
                    if (monitor.wait_for(lock, std::chrono::milliseconds(timeout)) == std::cv_status::timeout)
                        return false;
                }
            }
            return false;
        }

        inline bool operator>>(T &var) { return pop(var); }

        inline bool operator<<(T &var) { return push(var); }

        /**
     * Queue state
     */
        inline bool is_finished() const {
            return finalized;
        }

        /**
     * Close queue and release all waiting threads
     */
        inline void finish() {
            std::unique_lock<std::mutex> lock(mutex);
            finalized = true;
            monitor.notify_all();
        }

        /**
     * Close queue
     */
        ~BlockingQueue() {
            finish();
        }

    private:
        QUEUE queue_;
        volatile bool finalized = false;
        std::condition_variable monitor;
        std::mutex mutex;
    };


/**
 * Call something on destroy. For example thread::join
 */
    template<typename Fn>
    struct Defer {

        Defer(Fn &&fn) : fn(std::forward<Fn>(fn)) { }

        ~Defer() { fn(); }

    private:
        Fn fn;
    };

    template<typename Fn>
    Defer<Fn> defer(Fn &&fn) {
        return Defer<Fn>(std::forward<Fn>(fn));
    }

}
#endif //IO_CONCURRENT_H
