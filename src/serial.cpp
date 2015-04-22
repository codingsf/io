//
// Created by Red Dec on 21.04.15.
//

#include "serial.h"
#include <fcntl.h>

io::Serial::Serial(const std::string &path) : path_(path) {
    descriptor_ = open();
    if (descriptor_ < 0) {
        set_error();
        return;
    }
    set_auto_close(true);
}

bool io::Serial::open() {
    if (is_open_) {
        set_error(-1, "Already opened");
        return false;
    }
    descriptor_ = ::open(path_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (!has_valid_descriptor()) {
        set_error();
        return false;
    }
    fcntl(descriptor_, F_SETFL, 0); //Blocking model
    return (is_open_ = true);
}

bool io::Serial::set_baud_rate(uint32_t rate) {
    if (!has_valid_descriptor() || !is_open()) return false;
    struct termios options;
    if (tcgetattr(descriptor_, &options) < 0) {
        set_error();
        return false;
    }
    if (cfsetospeed(&options, rate) < 0 ||
        cfsetospeed(&options, rate) < 0) {
        set_error();
        return false;
    }
    options.c_cflag |= (CLOCAL | CREAD);
    if (tcsetattr(descriptor_, TCSAFLUSH, &options) < 0) {
        set_error();
        return false;
    }
    return true;
}

bool io::Serial::set_profile(Profile profile) {
    if (!has_valid_descriptor() || !is_open()) return false;
    struct termios options;
    if (tcgetattr(descriptor_, &options) < 0) {
        set_error();
        return false;
    }

    switch (profile) {
        case Profile::P_8N1: {
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS8;
            break;
        };
        case Profile::P_7E1: {
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            options.c_cflag &= ~CSTOPB;
            options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS7;
            break;
        };
        case Profile::P_7O1: {
            options.c_cflag |= PARENB;
            options.c_cflag |= PARODD;
            options.c_cflag &= ~CSTOPB;
            options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS7;
            break;
        };
        case Profile::P_7S1: {
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS8;
            break;
        };

    }
    if (tcsetattr(descriptor_, TCSAFLUSH, &options) < 0) {
        set_error();
        return false;
    }
    return true;
}

bool io::Serial::set_hardware_flow_control(bool enable) {
    if (!has_valid_descriptor() || !is_open()) return false;
    struct termios options;
    if (tcgetattr(descriptor_, &options) < 0) {
        set_error();
        return false;
    }
    if (enable)
        options.c_cflag |= CRTSCTS;
    else
        options.c_cflag &= ~CRTSCTS;
    if (tcsetattr(descriptor_, TCSAFLUSH, &options) < 0) {
        set_error();
        return false;
    }
    return true;
}
