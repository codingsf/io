//
// Created by Red Dec on 21.04.15.
//

#ifndef IO_SERIAL_H
#define IO_SERIAL_H

#include <termios.h>
#include "io.h"

namespace io {
    struct Serial : public io::Storage, public io::WithError {
        enum ErrCodes : int {
            InvalidBauds = -1,
            AlreadyOpened = -2
        };

        enum Profile {
            P_8N1, // No parity (8N1)
            P_7E1, // Even parity (7E1)
            P_7O1, // Odd parity (7O1)
            P_7S1  // Space parity
        };

        /**
         * Initialize resources and open
         */
        Serial(const std::string &path);

        /**
         * Open serial port
         */
        bool open();

        bool set_baud_rate(uint32_t rate);

        bool set_profile(Profile profile);

        bool set_hardware_flow_control(bool enable);

        bool set_blocking_mode(bool enable);

        inline bool is_open() const { return is_open_; }

        inline const std::string &path() const { return path_; }

    private:
        std::string path_;

        bool is_open_ = false;

        static speed_t get_speed_param(uint32_t speed);
    };


}
#endif //IO_SERIAL_H
