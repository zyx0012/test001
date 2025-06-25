#pragma once
#include <sstream>
#include <vector>
#include "./stream_buffer.hpp"

namespace coinpoker {
namespace protocol {
class StreamReader {
public:

    static StreamReader fromBytes(std::unique_ptr<char[]> data, std::size_t size) {
        return StreamReader(std::move(data), size);
    }

    int32_t readInt32();
    int16_t readInt16();
    int8_t readInt8();

    uint32_t readUInt32();
    uint16_t readUInt16();
    uint8_t readUInt8();

    bool readBool();
    int readSize();

    std::unique_ptr<char[]> readBytes(std::size_t n);
    std::unique_ptr<char[]> readAllBytes();

    std::string readString();
    StreamReader readCommMsg();

    void reset();
    void seek(std::streampos pos);
    void skip(std::streampos pos);
    std::streampos pos();
    StreamReader clone();


    inline unsigned int getLength() const {
        return length_;
    }

    inline std::istream& getStream() const {
        return *stream_;
    }

private:
      StreamReader(std::unique_ptr<char[]> data, std::size_t size)
        : buffer_(std::make_unique<StreamBuffer>(std::move(data), static_cast<std::ptrdiff_t>(size))),
          stream_(std::make_unique<std::istream>(buffer_.get())),
          length_(static_cast<unsigned int>(size)) {}


    std::unique_ptr<StreamBuffer> buffer_;
    std::unique_ptr<std::istream> stream_;
    std::size_t  length_;
};
}
}

