#ifndef BARCODE_READER_HPP
#define BARCODE_READER_HPP

#include <string>
#include <functional>
#include <atomic>
#include <thread>

namespace client {

// Callback type for when a barcode is scanned
using BarcodeCallback = std::function<void(const std::string& barcode)>;

class BarcodeReader {
public:
    explicit BarcodeReader(const std::string& device_path);
    ~BarcodeReader();

    // Start reading in background thread
    bool start(BarcodeCallback callback);

    // Stop reading
    void stop();

    // Check if reader is running
    bool isRunning() const { return running_; }

    // Get last error
    std::string getLastError() const { return last_error_; }

private:
    void readLoop();

    std::string device_path_;
    int fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread read_thread_;
    BarcodeCallback callback_;
    std::string last_error_;
    std::string current_barcode_;
};

} // namespace client

#endif // BARCODE_READER_HPP
