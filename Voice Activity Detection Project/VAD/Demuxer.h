#pragma once
#include <string>
#include <functional>

extern "C" {
#include <libavformat/avformat.h>
}

#include "Decoder.h"
#include "SwrContext.h"

namespace VAD {

	using fp_Demuxer_Callback_t = int(void*);

	class Demuxer {
		using FormatContext = std::unique_ptr<AVFormatContext, 
			std::function<void(AVFormatContext*)>>;
	public:
		Demuxer();
		~Demuxer();
		Demuxer(const Demuxer& r) = delete;
		Demuxer operator=(const Demuxer& r) = delete;

		int operator()(AVPacket* packet) const;

		AVFormatContext* operator->() const noexcept {
			return m_pFormatContext.get();
		}

		AVCodecParameters* getStreamCodecPar(int stream_index) const {
			return m_pFormatContext->streams[stream_index]->codecpar;
		};

		AVRational getStreamTimebase(int stream_index) const {
			return m_pFormatContext->streams[stream_index]->time_base;
		};

		AVStream* getStream(int stream_index) const {
			return m_pFormatContext->streams[stream_index];
		};

		int getSampleRate() const {
			return m_decoder.getSampleRate();
		}

		bool open(const std::string& filename, AVInputFormat* inputformat,
			AVDictionary** dict = nullptr, fp_Demuxer_Callback_t fn = nullptr, void* userdata = nullptr);

		bool isAborted() const {
			return m_bAborted;
		}

		void setAborted(bool aborted) {
			m_bAborted = aborted;
		}

		int getAudioStreamIndex() const {
			return m_iAudioStreamIndex;
		}

		int sendPacket(const AVPacket* packet);
		int recieveFrame(AVFrame* frame);

		int getDataSize() const;

		int convertDstAudio(SwrOptions src_opts, uint8_t** out, const uint8_t** in, int in_count);

		const SwrOptions& getSwrOptions() const {
			return m_convertContext.getDstSwrOptions();
		}

	private:
		bool m_bAborted{ false };
		int m_iAudioStreamIndex{ -1 };
		FormatContext m_pFormatContext;
		Decoder m_decoder;
		SwrConvertContext m_convertContext;
	};
}
