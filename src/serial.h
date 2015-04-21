//
// Created by Red Dec on 21.04.15.
//

#ifndef IO_SERIAL_H
#define IO_SERIAL_H

#include <termios.h>
#include "io.h"

namespace io {
    struct Serial : public io::Storage, public io::WithError {
        /**
         * Byte size
         */
        enum class Bits : uint32_t {
            CS_8 = CS8, CS_7 = CS7, CS_6 = CS6, CS_5 = CS5
        };

        /**
         * Control parity
         */
        enum class Parity : uint32_t {
            ODD = PARODD, MARK = PARMRK, NONE = PARENB
        };

        /**
         * Baud speed rate
         */
        enum class Speed : uint32_t {
            B_50 = B50,
            B_74 = B75,
            B_110 = B110,
            B_134 = B134,
            B_150 = B150,
            B_200 = B200,
            B_300 = B300,
            B_600 = B600,
            B_1200 = B1200,
            B_1800 = B1800,
            B_2400 = B2400,
            B_4800 = B4800,
            B_9600 = B9600,
            B_19200 = B19200,
            B_38400 = B38400
        };

        /**
         * Initialize resources and open
         */
        Serial(const std::string &path);

        /**
         * Set serial port attributes: baud rate, control parity and byte size
         */
        bool set_attributes(Speed speed = Speed::B_9600, Parity parity = Parity::NONE, Bits bits = Bits::CS_8);

        inline Speed speed() const { return speed_; }

        inline Parity parity() const { return parity_; }

        inline Bits bits() const { return bits_; }

    private:
        Speed speed_;
        Parity parity_;
        Bits bits_;
    };
}
#endif //IO_SERIAL_H
