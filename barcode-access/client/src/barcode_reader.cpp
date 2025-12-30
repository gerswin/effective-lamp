#include "barcode_reader.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#include <map>
#include <sys/ioctl.h>

namespace client {

// Key code to character mapping for standard US keyboard layout
static const std::map<int, char> KEY_MAP = {
    {KEY_1, '1'}, {KEY_2, '2'}, {KEY_3, '3'}, {KEY_4, '4'}, {KEY_5, '5'},
    {KEY_6, '6'}, {KEY_7, '7'}, {KEY_8, '8'}, {KEY_9, '9'}, {KEY_0, '0'},
    {KEY_MINUS, '-'},
    {KEY_A, 'a'}, {KEY_B, 'b'}, {KEY_C, 'c'}, {KEY_D, 'd'}, {KEY_E, 'e'},
    {KEY_F, 'f'}, {KEY_G, 'g'}, {KEY_H, 'h'}, {KEY_I, 'i'}, {KEY_J, 'j'},
    {KEY_K, 'k'}, {KEY_L, 'l'}, {KEY_M, 'm'}, {KEY_N, 'n'}, {KEY_O, 'o'},
    {KEY_P, 'p'}, {KEY_Q, 'q'}, {KEY_R, 'r'}, {KEY_S, 's'}, {KEY_T, 't'},
    {KEY_U, 'u'}, {KEY_V, 'v'}, {KEY_W, 'w'}, {KEY_X, 'x'}, {KEY_Y, 'y'},
    {KEY_Z, 'z'}
};

BarcodeReader::BarcodeReader(const std::string& device_path)
    : device_path_(device_path) {
}

BarcodeReader::~BarcodeReader() {
    stop();
}

bool BarcodeReader::start(BarcodeCallback callback) {
    if (running_) {
        return true;
    }

    callback_ = callback;

    // Open the input device
    fd_ = open(device_path_.c_str(), O_RDONLY);
    if (fd_ < 0) {
        last_error_ = "Failed to open device: " + device_path_ + " - " + strerror(errno);
        std::cerr << last_error_ << std::endl;
        return false;
    }

    // Grab the device exclusively (prevents other apps from seeing input)
    if (ioctl(fd_, EVIOCGRAB, 1) < 0) {
         std::cerr << "Warning: Failed to grab device exclusively: " << strerror(errno) << std::endl;
         // Proceed anyway, but warn
    } else {
         std::cout << "Device grabbed exclusively." << std::endl;
    }

    running_ = true;
    read_thread_ = std::thread(&BarcodeReader::readLoop, this);

    std::cout << "Barcode reader started on: " << device_path_ << std::endl;
    return true;
}

void BarcodeReader::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (fd_ >= 0) {
        // Release grab
        ioctl(fd_, EVIOCGRAB, 0);
        close(fd_);
        fd_ = -1;
    }

    if (read_thread_.joinable()) {
        read_thread_.join();
    }

    std::cout << "Barcode reader stopped" << std::endl;
}

#include <poll.h>

void BarcodeReader::readLoop() {
    struct input_event ev;
    current_barcode_.clear();

    struct pollfd fds;
    fds.fd = fd_;
    fds.events = POLLIN;

    bool shift_pressed = false;

    while (running_) {
        int ret = poll(&fds, 1, 100); // 100ms timeout
        if (ret < 0) {
            last_error_ = "Poll error: " + std::string(strerror(errno));
            std::cerr << last_error_ << std::endl;
            break;
        }
        if (ret == 0) continue;

        if (fds.revents & POLLIN) {
            ssize_t n = read(fd_, &ev, sizeof(ev));

            if (n < 0) {
                if (errno == EINTR) continue;
                last_error_ = "Read error: " + std::string(strerror(errno));
                std::cerr << last_error_ << std::endl;
                break;
            }

            if (n != sizeof(ev)) continue;

            if (ev.type != EV_KEY) continue;

            // Handle Shift keys (Pressed=1, Held=2, Released=0)
            if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
                if (ev.value == 1) shift_pressed = true;
                else if (ev.value == 0) shift_pressed = false;
                continue;
            }

            // Only process key presses (value 1)
            if (ev.value == 1) {
                if (ev.code == KEY_ENTER || ev.code == KEY_KPENTER) {
                    if (!current_barcode_.empty() && callback_) {
                        callback_(current_barcode_);
                        current_barcode_.clear();
                    }
                } else {
                    auto it = KEY_MAP.find(ev.code);
                    if (it != KEY_MAP.end()) {
                        char c = it->second;
                        // Handle Shift mappings
                        if (shift_pressed) {
                            if (c >= 'a' && c <= 'z') {
                                c = toupper(c);
                            } else if (c == '-') {
                                c = '_';
                            }
                            // Add other shift mappings if needed (e.g., numbers to symbols), 
                            // but NanoID only needs alphanumeric + _ and -
                        }
                        current_barcode_ += c;
                    }
                }
            }
        }
    }
}

} // namespace client
