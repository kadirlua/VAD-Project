#pragma once
#include <functional>
#include <memory>

/// Deleter adaptor for functions like av_free that take a pointer.
template<typename T, void (*Fn)(T*) >
struct CustomDeleter {
	inline void operator()(T* p) const noexcept {
		if (p)
			Fn(p);
	}
};

/// Deleter adaptor for functions like av_freep that take a pointer to a pointer.
template<typename T, void(*Fn)(T**) >
struct CustomDeleterP {
	inline void operator()(T* p) const noexcept {
		if (p)
			Fn(&p);
	}
};

struct AVFrame;
struct AVPacket;

using unique_AVFrame_ptr = std::unique_ptr<AVFrame, std::function<void(AVFrame*)>>;
using unique_AVPacket_ptr = std::unique_ptr<AVPacket, std::function<void(AVPacket*)>>;