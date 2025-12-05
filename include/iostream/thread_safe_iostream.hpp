#pragma once
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

inline std::mutex g_cin_mutex;
inline std::mutex g_cout_mutex;

class ThreadSafeIOStream
{
public:
    void setPrefix(const std::string& prefix);

    template <typename T> void prompt(const std::string& question, T& dest);

    void flush();
    // Overloaded operators
    template <typename T> ThreadSafeIOStream& operator<<(const T& value);
    template <typename T> ThreadSafeIOStream& operator>>(T& value);
    // Handle manipulators like std::endl, std::flush, ends, etc.
    ThreadSafeIOStream& operator<<(std::ostream& (*manip)(std::ostream&));

    // Handle manipulators like std::hex, std::dec, std::oct, etc.
    ThreadSafeIOStream& operator<<(std::ios& (*manip)(std::ios&));

private:
    void _flushCompletedLines();

private:
    std::string prefix_;
    std::ostringstream buffer_;
};

inline thread_local ThreadSafeIOStream threadSafeCout;
#define ts_cout    threadSafeCout
#define tsiostream threadSafeCout
