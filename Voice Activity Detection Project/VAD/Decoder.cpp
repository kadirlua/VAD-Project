#include "Decoder.h"

namespace VAD {

	Decoder::Decoder(AVCodec* codec):
		m_pAvCodecContext{ avcodec_alloc_context3(codec) }
	{

	}

	int Decoder::openCodec(const AVCodec* codec, AVDictionary** opts) const {
		return avcodec_open2(m_pAvCodecContext.get(), codec, opts);
	}

	void Decoder::setPktTimebase(AVRational timebase) {
		m_pAvCodecContext->pkt_timebase = timebase;
	}

	int Decoder::sendFrame(const AVFrame* frame) const {
		return avcodec_send_frame(m_pAvCodecContext.get(), frame);
	}

	int Decoder::receivePacket(AVPacket* packet) const {
		return avcodec_receive_packet(m_pAvCodecContext.get(), packet);
	}

	int Decoder::sendPacket(const AVPacket* packet) const {
		return avcodec_send_packet(m_pAvCodecContext.get(), packet);
	}

	int Decoder::receiveFrame(AVFrame* frame) const {
		return avcodec_receive_frame(m_pAvCodecContext.get(), frame);
	}
}