//
// Created by RedDec on 20.04.15.
//

#ifndef IO_EXPERIMENTAL_H
#define IO_EXPERIMENTAL_H

#include "io.h"
#include "async.h"
#include <sstream>
namespace io {


    struct Publisher : public io::AsyncSocketServer {

        Publisher(io::Epoll &epoll, io::ConnectionManager::Ptr serv_con_);

        inline std::stringstream &line() { return line_; }

        void publish();

    protected:
        std::stringstream line_;
    };

}
#endif //IO_EXPERIMENTAL_H
