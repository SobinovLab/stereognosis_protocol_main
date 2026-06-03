#pragma once
#include <windows.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include "Logger.h"


class ArduinoBreakBeamDetector {
 public:
  int detection_period_ms = 200;
  int timeout_ms = 200;
  int poll_sleep_ms = 5;
  int timeout_detect_ms = -1;  // no timeout

  explicit ArduinoBreakBeamDetector(std::string port = "", int baud = 115200)
      : port_(std::move(port)), baud_(baud) {}

  ~ArduinoBreakBeamDetector() { close(); }

  // Connect using provided port_ or auto-detect if empty.
  bool connect() {
    close();

    if (!port_.empty()) {
      if (!openPort(port_)) return false;
      if (!handshake()) {
        close();
        return false;
      }
      return true;
    }

    // Auto-detect: scan COM1..COM256
    for (int i = 1; i <= 256; ++i) {
      std::string candidate = "COM" + std::to_string(i);
      if (!openPort(candidate)) continue;

      if (handshake()) {
        port_ = candidate;
        return true;
      }
      close();
    }
    return false;
  }

  std::string port() const { return port_; }

  // Query once: true if beam was broken in the Arduino's last 500 ms window.
  bool isDetected() {
    if (!ensureConnected()) return false;
    writeLine("GET");
    std::string line = readLine(this->timeout_ms);
    return (line == "1");
  }

  // Clear the Arduino's break-event memory. Call between trials so leftover
  // state from the previous trial cannot satisfy the next one.
  void resetState() {
    if (!ensureConnected()) return;
    writeLine("RESET");
    try { readLine(this->timeout_ms); } catch (...) {}  // consume "OK"
  }

  // Poll until detected stays true for >= detection_period_ms.
  // timeoutMs < 0 means no timeout.
  bool waitDetectedStable(
      std::atomic<bool>& m_earnedReward,
      std::atomic<bool>& m_stopAsyncTrialConditionMonitor) {
    if (!ensureConnected()) return false;

    auto start = std::chrono::steady_clock::now();
    bool inDetection = false;
    auto detectStart = start;

    while (true) {
      if (m_stopAsyncTrialConditionMonitor.load()) return false;

      bool det = false;
      try {
        det = isDetected();
      } catch (...) {
        // If a read times out or errors, treat as not detected for robustness.
        det = false;
      }

      auto now = std::chrono::steady_clock::now();

      if (det) {
        if (!inDetection) {
          inDetection = true;
          detectStart = now;
        } else {
          auto durMs =
              (int)std::chrono::duration_cast<std::chrono::milliseconds>(
                  now - detectStart)
                  .count();
          if (durMs >= this->detection_period_ms) {
            m_earnedReward.store(true);
            return true;
          }
        }
      } else {
        inDetection = false;
      }

      if (this->timeout_detect_ms >= 0) {
        auto elapsedMs =
            (int)std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                       start)
                .count();
        if (elapsedMs >= this->timeout_detect_ms) return false;
      }

      std::this_thread::sleep_for(
          std::chrono::milliseconds(this->poll_sleep_ms));
    }
    return false;
  }

 private:
  HANDLE h_ = INVALID_HANDLE_VALUE;
  std::string port_;
  int baud_;

  int ensureConnected() const {
    if (h_ == INVALID_HANDLE_VALUE) {
      logError("Break-beam sensor not connected.");
      return -1;
    }
    return 0;
  }

  static std::string devicePath(const std::string& com) {
    // WinAPI requires \\.\COMx for COM10+
    return "\\\\.\\" + com;
  }

  bool openPort(const std::string& com) {
    std::string path = devicePath(com);

    h_ = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                     0,  // exclusive access
                     nullptr, OPEN_EXISTING, 0, nullptr);

    if (h_ == INVALID_HANDLE_VALUE) return false;

    // Configure
    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h_, &dcb)) {
      close();
      return false;
    }

    dcb.BaudRate = baud_;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    // Disable flow control
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl =
        DTR_CONTROL_ENABLE;  // often OK; opening may still reset Arduino
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    if (!SetCommState(h_, &dcb)) {
      close();
      return false;
    }

    // Timeouts (we do our own overall timeout in readLine)
    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout = 20;
    to.ReadTotalTimeoutConstant = 20;
    to.ReadTotalTimeoutMultiplier = 5;
    to.WriteTotalTimeoutConstant = 50;
    to.WriteTotalTimeoutMultiplier = 5;
    if (!SetCommTimeouts(h_, &to)) {
      close();
      return false;
    }

    PurgeComm(h_, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return true;
  }

  void close() {
    if (h_ != INVALID_HANDLE_VALUE) {
      CloseHandle(h_);
      h_ = INVALID_HANDLE_VALUE;
    }
  }

  bool handshake() {
    // Allow time for Arduino auto-reset after opening port
    std::this_thread::sleep_for(std::chrono::milliseconds(1600));
    PurgeComm(h_, PURGE_RXCLEAR | PURGE_TXCLEAR);

    writeLine("PING");
    try {
      std::string line = readLine(800);
      // Arduino may print READY first; tolerate that
      if (line.rfind("READY", 0) == 0) {
        // Read next line for PING response
        line = readLine(800);
      }
      return (line == "OK BREAKBEAM");
    } catch (...) {
      return false;
    }
  }

  void writeLine(const std::string& s) {
    std::string out = s + "\n";
    DWORD written = 0;
    if (!WriteFile(h_, out.data(), (DWORD)out.size(), &written, nullptr) ||
        written != out.size()) {
      throw std::runtime_error("WriteFile failed.");
    }
  }

  std::string readLine(int timeoutMs) {
    auto t0 = std::chrono::steady_clock::now();
    std::string buf;
    buf.reserve(64);

    while (true) {
      char ch;
      DWORD got = 0;
      BOOL ok = ReadFile(h_, &ch, 1, &got, nullptr);

      if (!ok) throw std::runtime_error("ReadFile failed.");

      if (got == 1) {
        if (ch == '\n') {
          // strip optional \r
          if (!buf.empty() && buf.back() == '\r') buf.pop_back();
          return buf;
        }
        buf.push_back(ch);
        if (buf.size() > 256) throw std::runtime_error("Line too long.");
      }

      auto now = std::chrono::steady_clock::now();
      int elapsed =
          (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - t0)
              .count();
      if (elapsed >= timeoutMs) throw std::runtime_error("Read timeout.");
    }
  }
};
