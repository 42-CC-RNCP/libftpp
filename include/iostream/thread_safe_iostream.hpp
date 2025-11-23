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
    void setPrefix(const std::string& prefix) { prefix_ = prefix; }

    template <typename T> void prompt(const std::string& question, T& dest)
    {
        // std::lock_guard only locks for the duration of its scope
        {
            // lock cout to print question
            std::lock_guard<std::mutex> lock(g_cout_mutex);
            std::cout << prefix_ << question << std::endl;
            // cout lock released here
        }
        {
            // lock cin to read answer
            std::lock_guard<std::mutex> lock(g_cin_mutex);
            std::cin >> dest;
            // cin lock released here
        }
        buffer_ << dest;
    }

    void flush()
    {
        std::lock_guard<std::mutex> lock(g_cout_mutex);

        _flushCompletedLines();
        // Flush any remaining partial line
        std::string remaining = buffer_.str();
        if (!remaining.empty()) {
            std::cout << prefix_ << remaining;
            buffer_.str(""); // Clear the buffer
            buffer_.clear(); // Clear any error flags
        }
    }

    // Overloaded operators
    template <typename T> ThreadSafeIOStream& operator<<(const T& value)
    {
        buffer_ << value;
        _flushCompletedLines();
        return *this;
    }

    template <typename T> ThreadSafeIOStream& operator>>(T& value)
    {
        {
            std::lock_guard<std::mutex> lock(g_cin_mutex);
            std::cin >> value;
        }
        buffer_ << value;
        return *this;
    }

    // Handle manipulators like std::endl, std::flush, ends, etc.
    ThreadSafeIOStream& operator<<(std::ostream& (*manip)(std::ostream&))
    {
        buffer_ << manip;
        _flushCompletedLines();
        return *this;
    }

    // Handle manipulators like std::hex, std::dec, std::oct, etc.
    ThreadSafeIOStream& operator<<(std::ios& (*manip)(std::ios&))
    {
        buffer_ << manip;
        return *this;
    }

private:
    void _flushCompletedLines()
    {
        std::string content = buffer_.str();
        size_t pos = 0;

        while ((pos = content.find('\n')) != std::string::npos) {
            std::string line = content.substr(0, pos);
            {
                std::lock_guard<std::mutex> lock(g_cout_mutex);
                std::cout << prefix_ << line << std::endl;
            }
            content.erase(0, pos + 1);
        }
        buffer_.str(content);
        buffer_.clear(); // Clear any error flags
        buffer_.seekp(0, std::ios::end); // Move the put pointer to the end
    }

private:
    std::string prefix_;
    std::ostringstream buffer_;
};

inline thread_local ThreadSafeIOStream threadSafeCout;
#define ts_cout threadSafeCout
#define tsiostream threadSafeCout
