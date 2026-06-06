#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

class Stream {
public:
    virtual ~Stream() = default;
    virtual int available() = 0;
    virtual int read() = 0;
};

class SerialStub {
public:
    template <typename T>
    void print(const T&) {}

    template <typename T>
    void println(const T&) {}

    void println() {}
};

inline SerialStub Serial;

class String {
public:
    String() = default;
    String(const char* value) : value(value ? value : "") {}
    String(const std::string& value) : value(value) {}

    int length() const
    {
        return static_cast<int>(value.length());
    }

    const char* c_str() const
    {
        return value.c_str();
    }

    bool startsWith(const char* prefix) const
    {
        const std::string prefixValue(prefix ? prefix : "");
        return value.rfind(prefixValue, 0) == 0;
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

    String substring(int start) const
    {
        if (start < 0 || static_cast<size_t>(start) >= value.length()) {
            return "";
        }

        return value.substr(static_cast<size_t>(start));
    }

    long toInt() const
    {
        return std::strtol(value.c_str(), nullptr, 10);
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

private:
    std::string value;
};
