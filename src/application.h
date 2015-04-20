//
// Created by Red Dec on 20.04.15.
//

#ifndef IO_APPLICATION_H
#define IO_APPLICATION_H

#include <functional>
#include <vector>
#include <unordered_map>

namespace io {
    struct Application {
        typedef std::function<void()> Callback;

        /**
         * Get current application
         */
        inline static Application &instance() {
            static Application application;
            return application;
        }

        /**
         * Register callbacks for signals
         */
        void add_signal_handler(int sig, const Callback &func);

        /**
         * Register callback on normal exit
         */
        void add_exit_handler(const Callback &func);

    private:
        void on_signal(int sig);

        void on_exit();

        static void signal_handler(int sig);

        static void exit_handler();

        Application() { }

        Application(const Application &) = delete;

        Application &operator=(const Application &) = delete;

        std::unordered_multimap<int, Callback> on_sig_;

        std::vector<Callback> on_exit_;
    };
}
#endif //IO_APPLICATION_H
