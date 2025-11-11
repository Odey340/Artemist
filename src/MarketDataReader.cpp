#include "MarketDataReader.hpp"
#include <fstream>
#include <cstring>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#include <handleapi.h>
#include <memoryapi.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

MarketDataReader::MarketDataReader(const std::string& filepath)
    : data_(nullptr), size_(0), position_(0), filepath_(filepath) {
    
#ifdef _WIN32
    HANDLE hFile = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }
    
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return;
    }
    size_ = fileSize.QuadPart;
    
    HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (hMap == nullptr) {
        CloseHandle(hFile);
        return;
    }
    
    data_ = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMap);
    CloseHandle(hFile);
#else
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        return;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return;
    }
    size_ = st.st_size;
    
    data_ = mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        size_ = 0;
    }
#endif
    
    // Skip header line if present
    if (data_ && size_ > 0) {
        const char* start = static_cast<const char*>(data_);
        const char* end = start + size_;
        const char* nl = static_cast<const char*>(memchr(start, '\n', size_));
        if (nl && nl < end) {
            position_ = (nl - start) + 1;
        }
    }
}

MarketDataReader::~MarketDataReader() {
    if (data_) {
#ifdef _WIN32
        UnmapViewOfFile(data_);
#else
        munmap(data_, size_);
#endif
    }
}

MarketDataReader::MarketDataReader(MarketDataReader&& other) noexcept
    : data_(other.data_), size_(other.size_), position_(other.position_), filepath_(std::move(other.filepath_)) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.position_ = 0;
}

MarketDataReader& MarketDataReader::operator=(MarketDataReader&& other) noexcept {
    if (this != &other) {
        if (data_) {
#ifdef _WIN32
            UnmapViewOfFile(data_);
#else
            munmap(data_, size_);
#endif
        }
        data_ = other.data_;
        size_ = other.size_;
        position_ = other.position_;
        filepath_ = std::move(other.filepath_);
        other.data_ = nullptr;
        other.size_ = 0;
        other.position_ = 0;
    }
    return *this;
}

bool MarketDataReader::parseLine(const char* line, size_t len, Tick& tick) {
    // Format: timestamp,bid,ask,volume
    std::string str(line, len);
    std::istringstream iss(str);
    std::string token;
    
    if (!std::getline(iss, token, ',')) return false;
    tick.timestamp = std::stoll(token);
    
    if (!std::getline(iss, token, ',')) return false;
    tick.bid = std::stod(token);
    
    if (!std::getline(iss, token, ',')) return false;
    tick.ask = std::stod(token);
    
    if (!std::getline(iss, token, ',')) return false;
    tick.volume = std::stoll(token);
    
    return true;
}

bool MarketDataReader::next(Tick& tick) {
    if (!data_ || position_ >= size_) {
        return false;
    }
    
    const char* start = static_cast<const char*>(data_);
    const char* current = start + position_;
    const char* end = start + size_;
    
    // Find next newline
    const char* nl = static_cast<const char*>(memchr(current, '\n', end - current));
    if (!nl) {
        nl = end;  // Last line may not have newline
    }
    
    if (nl <= current) {
        return false;
    }
    
    // Parse line
    size_t lineLen = nl - current;
    if (lineLen > 0 && current[lineLen - 1] == '\r') {
        lineLen--;  // Remove Windows line ending
    }
    
    if (lineLen > 0 && parseLine(current, lineLen, tick)) {
        position_ = (nl - start) + 1;
        return true;
    }
    
    position_ = (nl - start) + 1;
    return next(tick);  // Skip empty/invalid lines
}

void MarketDataReader::reset() {
    position_ = 0;
    // Skip header
    if (data_ && size_ > 0) {
        const char* start = static_cast<const char*>(data_);
        const char* nl = static_cast<const char*>(memchr(start, '\n', size_));
        if (nl) {
            position_ = (nl - start) + 1;
        }
    }
}

size_t MarketDataReader::approximateTickCount() const {
    if (!data_ || size_ == 0) return 0;
    // Rough estimate: assume average line is ~50 bytes
    return size_ / 50;
}

