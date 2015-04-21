//
// Created by Red Dec on 21.04.15.
//

#include "serial.h"
#include <fcntl.h>

io::Serial::Serial(const std::string &path) {
    descriptor_ = open(path.c_str(), O_RDWR | O_NOCTTY);
    if (descriptor_ < 0) {
        set_error();
        return;
    }
    set_auto_close(true);
    set_attributes();
}

bool io::Serial::set_attributes(Speed speed, Parity parity, Bits bits) {
    if (has_error() || !has_valid_descriptor())return false;
    struct termios tty;
    memset(&tty, 0, sizeof tty);

    /* Error Handling */
    if (tcgetattr(descriptor_, &tty) != 0) {
        set_error();
        return false;
    }
    /* Set Baud Rate */
    cfsetospeed(&tty, (speed_t) B9600);
    cfsetispeed(&tty, (speed_t) B9600);

    /* Setting other Port Stuff */
    tty.c_cflag &= ~static_cast<uint32_t>(parity);               // Make 8n1
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= static_cast<uint32_t>(bits);

    tty.c_cflag &= ~CRTSCTS;              // no flow control
    tty.c_cc[VMIN] = 1;                   // read doesn't block
    tty.c_cc[VTIME] = 1;                  // 1 seconds read timeout
    tty.c_cflag |= CREAD | CLOCAL;        // turn on READ & ignore ctrl lines

    return true;
}