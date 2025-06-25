#pragma once
#include <sstream>

namespace coinpoker {
namespace protocol {

class StreamBuffer : public std::streambuf {

public:
	StreamBuffer(std::unique_ptr<char[]> data, std::ptrdiff_t n): data_(std::move(data)) {
		setg(data_.get(), data_.get(), data_.get() + n);
	}

protected:
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir dir,
		std::ios_base::openmode which) override {
		if (which != std::ios_base::in) return -1;

		char* new_gptr = nullptr;

		if (dir == std::ios_base::beg) {
			new_gptr = eback() + off;
		}
		else if (dir == std::ios_base::cur) {
			new_gptr = gptr() + off;
		}
		else if (dir == std::ios_base::end) {
			new_gptr = egptr() + off;
		}

		if (new_gptr < eback() || new_gptr > egptr()) return -1;

		setg(eback(), new_gptr, egptr());
		return gptr() - eback();
	}

	std::streampos seekpos(std::streampos pos, std::ios_base::openmode which) override {
		return seekoff(pos, std::ios_base::beg, which);
	}
private:
    std::unique_ptr<char[]> data_;
};
}
}
