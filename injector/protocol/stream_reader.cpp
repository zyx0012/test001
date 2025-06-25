#include "./stream_reader.hpp"

namespace coinpoker {
namespace protocol {


int32_t StreamReader::readInt32() {
	unsigned char bytes[4];
	stream_->read(reinterpret_cast<char*>(bytes), sizeof(bytes));
	if (stream_->gcount() != sizeof(bytes)) {
		throw std::runtime_error("Error: Unable to read 4 bytes for int32");
	}

	return (static_cast<int32_t>(bytes[3]) << 24) |
		(static_cast<int32_t>(bytes[2]) << 16) |
		(static_cast<int32_t>(bytes[1]) << 8) |
		(static_cast<int32_t>(bytes[0]));
}

uint32_t StreamReader::readUInt32() {
	unsigned char bytes[4];
	stream_->read(reinterpret_cast<char*>(bytes), sizeof(bytes));
	if (stream_->gcount() != sizeof(bytes)) {
		throw std::runtime_error("Error: Unable to read 4 bytes for uint32");
	}

	return (static_cast<uint32_t>(bytes[3]) << 24) |
		(static_cast<uint32_t>(bytes[2]) << 16) |
		(static_cast<uint32_t>(bytes[1]) << 8) |
		(static_cast<uint32_t>(bytes[0]));
}

std::streampos StreamReader::pos() {
	return stream_->tellg();
}

int16_t StreamReader::readInt16() {
	unsigned char bytes[2];
	stream_->read(reinterpret_cast<char*>(bytes), sizeof(bytes));
	if (stream_->gcount() != sizeof(bytes)) {
		throw std::runtime_error("Error: Unable to read 2 bytes for int16");
	}

	return (static_cast<int16_t>(bytes[1]) << 8) |
		(static_cast<int16_t>(bytes[0]));
}

uint16_t StreamReader::readUInt16() {
	unsigned char bytes[2];
	stream_->read(reinterpret_cast<char*>(bytes), sizeof(bytes));
	if (stream_->gcount() != sizeof(bytes)) {
		throw std::runtime_error("Error: Unable to read 2 bytes for uint16");
	}

	return (static_cast<uint16_t>(bytes[1]) << 8) |
		(static_cast<uint16_t>(bytes[0]));
}

bool StreamReader::readBool() {
	char byte;
	stream_->read(&byte, 1);
	if (stream_->gcount() != 1) {
		throw std::runtime_error("Error: Unable to read 1 byte for bool");
	}
	return byte != 0;
}

int StreamReader::readSize() {
	uint8_t a = readUInt8();
	if ((a & 0x1) == 0x1) {
		return (a - 1) >> 1;
	}
	else if ((a & 0x3) == 0x2) {
		uint8_t b = readUInt8();
		return (((b << 8) | a) - 2) >> 2;
	}
	else {
		throw std::runtime_error("Error: Unexpected tag");
	}
}

std::string StreamReader::readString() {
	int size = readSize();
	auto buffer = readBytes(size);
	return std::string(buffer.get(), size);
}

std::int8_t StreamReader::readInt8() {
	int8_t result;
	stream_->read(reinterpret_cast<char*>(&result), 1);
	return result;
}

std::uint8_t StreamReader::readUInt8() {
	uint8_t result;
	stream_->read(reinterpret_cast<char*>(&result), 1);
	return result;
}

std::unique_ptr<char[]> StreamReader::readBytes(std::size_t n) {
	std::unique_ptr<char[]> buffer(new char[n]);
	stream_->read(buffer.get(), n);
	if (static_cast<std::size_t>(stream_->gcount()) != n)
		throw std::runtime_error("Error: Unable to read the requested number of bytes");
	return buffer;
}


std::unique_ptr<char[]> StreamReader::readAllBytes() {
	auto currentPos = stream_->tellg();
	reset();
	auto result = readBytes(getLength());
	skip(currentPos);
	return result;
}

void StreamReader::reset() {
	stream_->clear();
	stream_->seekg(0, std::ios::beg);
}

StreamReader StreamReader::clone() {
	auto newData = readAllBytes();
	return StreamReader::fromBytes(std::move(newData), length_);
}

void StreamReader::seek(std::streampos i) {
	if (i < 0 || static_cast<unsigned int>(i) > length_) {
		throw std::out_of_range("Seek position is out of bounds");
	}
	stream_->clear();
	stream_->seekg(i, std::ios::beg);
}

void StreamReader::skip(std::streampos i) {
	seek(stream_->tellg() + i);
}

}
}
