#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <windows.h>
#include <iostream>
using namespace std;

std::string getCurrentTime() {
    // Get current time as time_point
    auto now = std::chrono::system_clock::now();
    auto now_as_time_t = std::chrono::system_clock::to_time_t(now);

    // Convert to tm struct for formatting
    std::tm now_tm = *std::localtime(&now_as_time_t);

    // Get milliseconds
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Create a stringstream and format the time
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

    return ss.str();
}

int main() {
    std::ofstream outFile("output.txt", std::ios::out | std::ios::app);
    if (!outFile.is_open()) {
        return -1;
    }

    std::vector<std::string> buffer;
    buffer.reserve(2000); // Reserve space for 2000 timestamps

    HANDLE h_Serial;
    h_Serial = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 0);
    if (h_Serial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            // serial port not found. Handle error here.
            cout << "Serial port not found";
            return -1;
        }
        // any other error. Handle error here.
        cout << "Error opening serial port to read";
        return -1;
    }

    DCB dcbSerialParam = {0};
    dcbSerialParam.DCBlength = sizeof(dcbSerialParam);

    if (!GetCommState(h_Serial, &dcbSerialParam)) {
    // handle error here
    }

    dcbSerialParam.BaudRate = CBR_19200;
    dcbSerialParam.ByteSize = 8;
    dcbSerialParam.StopBits = ONESTOPBIT;
    dcbSerialParam.Parity = NOPARITY;

    if (!SetCommState(h_Serial, &dcbSerialParam)) {
    // handle error here
    }

    COMMTIMEOUTS timeout = {0};
    timeout.ReadIntervalTimeout = 60;
    timeout.ReadTotalTimeoutConstant = 60;
    timeout.ReadTotalTimeoutMultiplier = 15;
    timeout.WriteTotalTimeoutConstant = 60;
    timeout.WriteTotalTimeoutMultiplier = 8;
    if (!SetCommTimeouts(h_Serial, &timeout)) {
    // handle error here
    }

    auto next = std::chrono::high_resolution_clock::now();
    while (true) {
        // Time control logic
        auto start = std::chrono::high_resolution_clock::now();
        if (start < next) {
            continue;
        }
        next = start + std::chrono::microseconds(500); // Update next target time for 2000 Hz
            
        // Generate time   stamp with milliseconds
        std::string timestamp = getCurrentTime();
        buffer.push_back(timestamp);

        // Write when buffer is full
        if (buffer.size() >= 2000) {
            for (const auto& ts : buffer) {
                outFile << ts << std::endl;
            }
            buffer.clear();
        }
    }
    for (const auto& ts : buffer) {
        outFile << ts << std::endl;
    }
    buffer.clear();
    outFile.close();

    // The loop is infinite; buffer is flushed periodically, not on exit.
    return 0;
}
