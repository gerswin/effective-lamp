#include "barcode_reader.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#include <map>

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

    // Grab the device exclusively (optional, prevents other apps from seeing input)
    // Uncomment if you want exclusive access:
    // ioctl(fd_, EVIOCGRAB, 1);

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
        close(fd_);
        fd_ = -1;
    }

    if (read_thread_.joinable()) {
        read_thread_.join();
    }

    std::cout << "Barcode reader stopped" << std::endl;
}

void BarcodeReader::readLoop() {
    struct input_event ev;
    current_barcode_.clear();

    while (running_) {
        ssize_t n = read(fd_, &ev, sizeof(ev));

        if (n < 0) {
            if (errno == EINTR) continue;
            last_error_ = "Read error: " + std::string(strerror(errno));
            std::cerr << last_error_ << std::endl;
            break;
        }

        if (n != sizeof(ev)) {
            continue;
        }

        // Only process key events
        if (ev.type != EV_KEY) {
            continue;
        }

        // Only process key press events (value 1 = press, 0 = release, 2 = repeat)
        if (ev.value != 1) {
            continue;
        }

        // Enter key signals end of barcode
        if (ev.code == KEY_ENTER || ev.code == KEY_KPENTER) {
            if (!current_barcode_.empty() && callback_) {
                std::cout << "Barcode scanned: " << current_barcode_ << std::endl;
                callback_(current_barcode_);
                current_barcode_.clear();
            }
            continue;
        }

        // Map key code to character
        auto it = KEY_MAP.find(ev.code);
        if (it != KEY_MAP.end()) {
            current_barcode_ += it->second;
        }
    }
}

} // namespace client
