//
// Created by Red Dec on 21.04.15.
//

#include "serial.h"
#include <fcntl.h>
#include <unordered_map>

io::Serial::Serial(const std::string &path) : path_(path) {
    set_auto_close(true);
}

bool io::Serial::open(uint32_t flags) {
    if (is_open_) {
        set_error(ErrCodes::AlreadyOpened, "Already opened");
        return false;
    }
    descriptor_ = ::open(path_.c_str(), flags);
    if (!has_valid_descriptor()) {
        set_error();
        return false;
    }
    fcntl(descriptor_, F_SETFL, 0); //Blocking model
    return (is_open_ = true);
}

bool io::Serial::set_baud_rate(uint32_t rate) {
    if (!has_valid_descriptor() || !is_open()) return false;
    if ((rate = get_speed_param(rate)) == B0) {
        set_error(ErrCodes::InvalidBauds, "Invalid baud rate");
        return false;
    }
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
    if (tcsetattr(descriptor_, TCSANOW, &options) < 0) {
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

speed_t io::Serial::get_speed_param(uint32_t speed) {
    static const std::unordered_map<uint32_t, speed_t> speeds = {
            {50,     B50},
            {75,     B75},
            {110,    B110},
            {134,    B134},
            {150,    B150},
            {200,    B200},
            {300,    B300},
            {600,    B600},
            {1200,   B1200},
            {1800,   B1800},
            {2400,   B2400},
            {4800,   B4800},
            {9600,   B9600},
            {19200,  B19200},
            {38400,  B38400},
            {57600,  B57600},
            {115200, B115200},
    };
    auto r = speeds.find(speed);
    if (r != speeds.end()) return (*r).second;
    return B0;
}

bool io::Serial::set_blocking_mode(bool enable) {
    if (!has_valid_descriptor() || !is_open()) return false;
    if (enable) fcntl(descriptor_, F_SETFL, 0);
    else fcntl(descriptor_, F_SETFL, FNDELAY);
    return true;

}
