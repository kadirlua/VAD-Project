#include "Demuxer.h"

namespace VAD {

	static inline int abortDemuxer(void* p)
	{
		const Demuxer* pDemuxer = static_cast<const Demuxer*>(p);
		return pDemuxer->isAborted();
	};

	Demuxer::Demuxer() :
		m_decoder{ nullptr }
	{
		
	}

	Demuxer::~Demuxer() {
		//abort the demuxer
		m_bAborted = true;
	}

	bool Demuxer::open(const std::string& filename, AVInputFormat* inputformat, AVDictionary** dict, 
		fp_Demuxer_Callback_t fn, void* userdata)
	{
		AVCodec* codec = nullptr;
		AVFormatContext* context = avformat_alloc_context();
		if (!context)
			return false;

		/*
			we should set the callback proc before open the media
			to prevent hanging
		*/
		context->interrupt_callback.callback = fn ? fn : abortDemuxer;
		context->interrupt_callback.opaque = userdata ? userdata : this;
		
		int ret = avformat_open_input(&context, filename.c_str(), inputformat, dict);
		if (ret < 0)
			return false;

		auto free_context = [](AVFormatContext* ctx) {
			avformat_close_input(&ctx);
		};
		
		m_pFormatContext = std::unique_ptr<AVFormatContext,
			decltype(free_context)>(context,
				free_context);

		av_format_inject_global_side_data(m_pFormatContext.get());

		avformat_find_stream_info(m_pFormatContext.get(), dict);

		//find best stream index for audio
		m_iAudioStreamIndex = av_find_best_stream(m_pFormatContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

		if (m_iAudioStreamIndex < 0 && m_iAudioStreamIndex >= (int)m_pFormatContext->nb_streams)
			return false;

		auto pCodecPar = getStreamCodecPar(m_iAudioStreamIndex);
		auto pStream = getStream(m_iAudioStreamIndex);

		codec = avcodec_find_decoder(pCodecPar->codec_id);
		if (!codec)
			return false; //nothing found

		ret = m_decoder.parameterstoContext(pCodecPar);
		if (ret < 0)
			return false;

		m_decoder->codec_id = codec->id;
		m_decoder.setPktTimebase(pStream->time_base);

		if (m_decoder.openCodec(codec, dict) < 0)
			return false;

		pStream->discard = AVDISCARD_DEFAULT;

		return true;
	}

	int Demuxer::sendPacket(const AVPacket* packet)
	{
		return m_decoder.sendPacket(packet);
	}

	int Demuxer::recieveFrame(AVFrame* frame)
	{
		return m_decoder.receiveFrame(frame);
	}

	int Demuxer::operator()(AVPacket* packet) const
	{
		return av_read_frame(m_pFormatContext.get(), packet);
	}

	int Demuxer::getDataSize() const 
	{
		return m_decoder.getDataSize();
	}

	int Demuxer::convertDstAudio(SwrOptions src_opts, uint8_t** out, const uint8_t** in, int in_count)
	{
		const auto& opts = m_convertContext.getDstSwrOptions();

		if (!m_convertContext.init(src_opts))
			return 0;

		const int out_num_samples = av_rescale_rnd(swr_get_delay(m_convertContext.getContext(), in_count) + 
			in_count, opts.sample_rate, in_count, AV_ROUND_UP);

		if (av_samples_alloc(out, nullptr, av_get_channel_layout_nb_channels(opts.ch_layout), 
			out_num_samples, opts.sample_fmt, 0) < 0)
			return 0;
		
		int len = m_convertContext.convert(out, out_num_samples, in, in_count);

		if (len < 0)
		{
			return -1;
		}

		if (len == out_num_samples) {
			//audio buffer is probably too small
			m_convertContext.init(src_opts);
		}

		return len * av_get_channel_layout_nb_channels(opts.ch_layout) *
			av_get_bytes_per_sample(opts.sample_fmt);;
	}
}