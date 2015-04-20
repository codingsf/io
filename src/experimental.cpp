//
// Created by RedDec on 20.04.15.
//

#include "experimental.h"
#include <string>

namespace io {

    Publisher::Publisher(io::Epoll &epoll, io::ConnectionManager::Ptr serv_con_) : AsyncSocketServer(epoll,
                                                                                                     serv_con_) { }

    void Publisher::publish() {
        std::string content = line_.str();
        auto lock = lock_collection();
        for (auto &kv:clients_) {
            kv.second->output() << content;
            kv.second->output().flush();
        }
        line_.str("");
        line_.clear();
    }
}