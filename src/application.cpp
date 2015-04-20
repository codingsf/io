//
// Created by Red Dec on 20.04.15.
//

#include "application.h"
#include <csignal>
#include <cstdlib>

namespace io {

    void Application::add_signal_handler(int sig, const Callback &func) {
        if (on_sig_.find(sig) == on_sig_.end())
            signal(sig, &Application::signal_handler);
        on_sig_.insert(std::make_pair(sig, func));
    }

    void Application::add_exit_handler(const Callback &func) {
        if (on_exit_.empty()) atexit(&Application::exit_handler);
        on_exit_.push_back(func);
    }

    void Application::on_signal(int sig) {
        auto range = on_sig_.equal_range(sig);
        for (auto it = range.first; it != range.second; ++it) {
            (*it).second();
        }
    }

    void Application::on_exit() {
        for (auto &callback:on_exit_) callback();
    }

    void Application::signal_handler(int sig) { instance().on_signal(sig); }

    void Application::exit_handler() { instance().on_exit(); }
}