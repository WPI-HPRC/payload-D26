#pragma once

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>

using byte = uint8_t;

inline unsigned long fakeMillis = 0;

inline unsigned long millis()
{
    return fakeMillis;
}

template <typename T>
T min(T lhs, T rhs)
{
    return lhs < rhs ? lhs : rhs;
}

template <typename T>
T max(T lhs, T rhs)
{
    return lhs > rhs ? lhs : rhs;
}

template <typename T>
T constrain(T value, T lower, T upper)
{
    if (value < lower) {
        return lower;
    }

    if (value > upper) {
        return upper;
    }

    return value;
}

class String {
public:
    String() = default;
    String(const char* value) : value(value ? value : "") {}
    String(const std::string& value) : value(value) {}
    String(int value) : value(std::to_string(value)) {}
    String(unsigned int value) : value(std::to_string(value)) {}
    String(long value) : value(std::to_string(value)) {}
    String(unsigned long value) : value(std::to_string(value)) {}

    int length() const
    {
        return static_cast<int>(value.length());
    }

    const char* c_str() const
    {
        return value.c_str();
    }

    void reserve(size_t capacity)
    {
        value.reserve(capacity);
    }

    bool startsWith(const char* prefix) const
    {
        const std::string prefixValue(prefix ? prefix : "");
        return value.rfind(prefixValue, 0) == 0;
    }

    bool startsWith(const String& prefix) const
    {
        return value.rfind(prefix.value, 0) == 0;
    }

    bool endsWith(const char* suffix) const
    {
        const std::string suffixValue(suffix ? suffix : "");

        if (suffixValue.length() > value.length()) {
            return false;
        }

        return value.compare(value.length() - suffixValue.length(), suffixValue.length(), suffixValue) == 0;
    }

    void replace(const char* from, const char* to)
    {
        const std::string fromValue(from ? from : "");
        const std::string toValue(to ? to : "");

        if (fromValue.empty()) {
            return;
        }

        size_t pos = 0;
        while ((pos = value.find(fromValue, pos)) != std::string::npos) {
            value.replace(pos, fromValue.length(), toValue);
            pos += toValue.length();
        }
    }

    int indexOf(char character) const
    {
        const size_t position = value.find(character);

        if (position == std::string::npos) {
            return -1;
        }

        return static_cast<int>(position);
    }

    int indexOf(char character, int fromIndex) const
    {
        if (fromIndex < 0 || static_cast<size_t>(fromIndex) >= value.length()) {
            return -1;
        }

        const size_t position = value.find(character, static_cast<size_t>(fromIndex));

        if (position == std::string::npos) {
            return -1;
        }

        return static_cast<int>(position);
    }

    String substring(int start) const
    {
        if (start < 0 || static_cast<size_t>(start) >= value.length()) {
            return "";
        }

        return value.substr(static_cast<size_t>(start));
    }

    String substring(int start, int end) const
    {
        if (start < 0) {
            start = 0;
        }

        if (end < start) {
            end = start;
        }

        if (static_cast<size_t>(start) >= value.length()) {
            return "";
        }

        size_t cappedEnd = std::min(static_cast<size_t>(end), value.length());
        return value.substr(static_cast<size_t>(start), cappedEnd - static_cast<size_t>(start));
    }

    void trim()
    {
        const size_t start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            value.clear();
            return;
        }

        const size_t end = value.find_last_not_of(" \t\r\n");
        value = value.substr(start, end - start + 1);
    }

    void toLowerCase()
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    }

    long toInt() const
    {
        return std::strtol(value.c_str(), nullptr, 10);
    }

    float toFloat() const
    {
        return std::strtof(value.c_str(), nullptr);
    }

    String& operator=(const char* rhs)
    {
        value = rhs ? rhs : "";
        return *this;
    }

    String& operator=(const String& rhs) = default;

    String& operator+=(const String& rhs)
    {
        value += rhs.value;
        return *this;
    }

    String& operator+=(char rhs)
    {
        value += rhs;
        return *this;
    }

    bool operator==(const char* rhs) const
    {
        return value == (rhs ? rhs : "");
    }

    bool operator!=(const char* rhs) const
    {
        return !(*this == rhs);
    }

    bool operator==(const String& rhs) const
    {
        return value == rhs.value;
    }

    bool operator!=(const String& rhs) const
    {
        return !(*this == rhs);
    }

private:
    std::string value;

    friend class Stream;
};

class Stream {
public:
    virtual ~Stream() = default;
    virtual int available() { return 0; }
    virtual int read() { return -1; }
    virtual size_t write(uint8_t) { return 1; }

    size_t write(const char* value)
    {
        if (value == nullptr) {
            return 0;
        }

        size_t count = 0;
        while (*value != '\0') {
            count += write(static_cast<uint8_t>(*value));
            value++;
        }
        return count;
    }

    size_t write(const uint8_t* value, size_t size)
    {
        size_t count = 0;
        for (size_t i = 0; i < size; i++) {
            count += write(value[i]);
        }
        return count;
    }

    void print(const char* value) { write(value); }
    void print(char value) { write(static_cast<uint8_t>(value)); }
    void print(const String& value) { write(value.value.c_str()); }
    void print(int value) { printNumber(value); }
    void print(unsigned int value) { printNumber(value); }
    void print(long value) { printNumber(value); }
    void print(unsigned long value) { printNumber(value); }
    void print(float value, int digits = 2) { printFloat(value, digits); }
    void print(double value, int digits = 2) { printFloat(value, digits); }

    void println() { write(static_cast<uint8_t>('\n')); }

    template <typename T>
    void println(const T& value)
    {
        print(value);
        println();
    }

private:
    template <typename T>
    void printNumber(T value)
    {
        std::ostringstream output;
        output << value;
        write(output.str().c_str());
    }

    void printFloat(double value, int digits)
    {
        std::ostringstream output;
        output << std::fixed << std::setprecision(digits) << value;
        write(output.str().c_str());
    }
};

class SerialStub : public Stream {
public:
    size_t write(uint8_t) override { return 1; }
};

inline SerialStub Serial;
