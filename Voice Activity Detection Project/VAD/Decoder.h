#pragma once
#include "CustomDeleter.h"
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace VAD {

	class Decoder {
		using AVContext = std::unique_ptr<AVCodecContext, 
			CustomDeleterP<AVCodecContext, avcodec_free_context>>;
	public:
		Decoder(AVCodec *codec);
		~Decoder() = default;
		Decoder(const Decoder& r) = delete;
		Decoder& operator=(const Decoder& r) = delete;

		AVCodecContext* operator->() noexcept {
			return m_pAvCodecContext.get();
		}

		int Decoder::parameterstoContext(const AVCodecParameters* par) {
			return avcodec_parameters_to_context(m_pAvCodecContext.get(), par);
		}

		int getSampleRate() const {
			return m_pAvCodecContext->sample_rate;
		}

		int getDataSize() const {
			return av_get_bytes_per_sample(m_pAvCodecContext->sample_fmt);
		}

		void setPktTimebase(AVRational timebase);

		int openCodec(const AVCodec* codec, AVDictionary** opts = nullptr) const;

		int sendFrame(const AVFrame* frame) const;
		int receivePacket(AVPacket* packet) const;
		int sendPacket(const AVPacket* packet) const;
		int receiveFrame(AVFrame* frame) const;
	private:
		AVContext m_pAvCodecContext;
	};
}