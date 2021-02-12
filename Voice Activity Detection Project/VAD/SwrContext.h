#pragma once
#include <memory>
#include <functional>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

namespace VAD {

	struct SwrOptions {
		uint64_t ch_layout{};
		int sample_rate{};
		AVSampleFormat sample_fmt{};

		bool operator!=(const SwrOptions& r) const
		{
			return ch_layout != r.ch_layout || sample_rate != r.sample_rate || sample_fmt != r.sample_fmt;
		}
	};

	class SwrConvertContext {
		static constexpr auto DEFAULT_SAMPLE_RATE = 16000;
		using ConvertContext = std::unique_ptr<SwrContext,
			std::function<void(SwrContext*)>>;
	public:
		SwrConvertContext();
		~SwrConvertContext();

		SwrConvertContext(const SwrConvertContext& r) = delete;
		SwrConvertContext& operator=(const SwrConvertContext& r) = delete;

		SwrContext* getContext() const noexcept {
			return m_swrCtx.get();
		}

		void setSwrOptions(SwrOptions dst);

		const SwrOptions& getDstSwrOptions() const {
			return m_dstSwrOpts;
		}

		bool init(SwrOptions src);

		int convert(uint8_t** out, int out_count,
			const uint8_t** in, int in_count);

	private:
		SwrOptions m_dstSwrOpts{ AV_CH_LAYOUT_MONO, DEFAULT_SAMPLE_RATE, AV_SAMPLE_FMT_S16 };
		ConvertContext m_swrCtx;
	};
}