#include "SpeechDetector.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <speechapi_cxx.h>

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;

namespace VAD {

	static inline std::string timetoStr(double time) {
		int tns_int = static_cast<int>(time);
		int thh = tns_int / 3600;
		int tmm = (tns_int % 3600) / 60;
		int tss = (tns_int % 60);
		std::stringstream str;
		str.fill('0');
		str << std::setw(2) << thh << ":" << std::setw(2) << tmm << ":" << std::setw(2) << tss;
		return str.str();
	}

	SpeechDetector::SpeechDetector():
		m_MsgQueue{ MSG_QUEUE_SIZE }
	{

	}

	SpeechDetector::~SpeechDetector() {

		if (m_recognitionEnd.get_future().valid())
			m_recognitionEnd.set_value(); // Notify to stop recognition.

		m_MsgQueue.quit();

		if (m_MessageQueueThread && m_MessageQueueThread->joinable())
			m_MessageQueueThread->join();

		if (m_ReadFileThread && m_ReadFileThread->joinable())
			m_ReadFileThread->join();
	}

	bool SpeechDetector::createVAD(const std::string& filePath, fp_Demuxer_Callback_t fn, void* userdata)
	{
		if (m_demuxer.open(filePath, nullptr, nullptr, fn, userdata))
		{
			return true;
		}
		return false;
	}

	bool SpeechDetector::createMessageQueue(const fp_MessageLoopCallBack_t& msg_loop)
	{
		if (!msg_loop)
			return false;

		m_MessageQueueThread = std::make_unique<std::thread>(msg_loop, this);

		if (m_MessageQueueThread)
			return true;

		return false;
	}

	void SpeechDetector::ReadIOThreadProc()
	{	
		int ret{};
		auto pkt = unique_AVPacket_ptr(av_packet_alloc(),
			[](AVPacket* p) { av_packet_free(&p); });

		auto frame = unique_AVFrame_ptr(av_frame_alloc(),
			[](AVFrame* f) { av_frame_free(&f); });

		// Creates an instance of a speech config with specified subscription key and service region.
		// Replace with your own subscription key and service region (e.g., "westus").
		auto config = SpeechConfig::FromSubscription("2bf5e602161145678a0ad3d88d481aae", "westus");

		/*auto autoDetectSourceLanguageConfig =
			AutoDetectSourceLanguageConfig::FromLanguages({ "en-US", "tr-TR" });*/

		//config->SetSpeechRecognitionLanguage("tr-TR");

		// Creates a push stream
		auto pushStream = AudioInputStream::CreatePushStream();

		// Creates a speech recognizer from stream input;
		auto audioInput = AudioConfig::FromStreamInput(pushStream);
		auto recognizer = SpeechRecognizer::FromConfig(config, /*autoDetectSourceLanguageConfig,*/ audioInput);

		// Subscribes to events.
		recognizer->Recognizing.Connect([this](const SpeechRecognitionEventArgs& e)
			{
				//sendNotifyMessage(MSG_PUSH, e.Result->Text.c_str());
			});

		recognizer->Recognized.Connect([this](const SpeechRecognitionEventArgs& e)
			{
				if (e.Result->Reason == ResultReason::RecognizedSpeech)
				{
					auto offset = std::chrono::duration<double>(
						std::chrono::nanoseconds(e.Result->Offset() * 100));

					auto duration = std::chrono::duration<double>(
						std::chrono::nanoseconds(e.Result->Duration() * 100));

					std::string strText;
					strText += timetoStr(offset.count());
					strText += " - ";
					strText += timetoStr(offset.count() + duration.count());
					strText += "  ";
					strText += e.Result->Text;
					sendNotifyMessage(MSG_PUSH, strText.c_str());
				}
				else if (e.Result->Reason == ResultReason::NoMatch)
				{
					sendNotifyMessage(MSG_PUSH, "NOMATCH: Speech could not be recognized.");
				}
			});

		recognizer->Canceled.Connect([this](const SpeechRecognitionCanceledEventArgs& e)
			{
				switch (e.Reason)
				{
				case CancellationReason::EndOfStream:
					sendNotifyMessage(MSG_PUSH, "Reach the end of the file.");
					break;

				case CancellationReason::Error:
					sendNotifyMessage(MSG_PUSH, e.ErrorDetails.c_str());
					m_recognitionEnd.set_value();
					break;

				default:
					sendNotifyMessage(MSG_PUSH, "CANCELED: received unknown reason.");
				}

				sendNotifyMessage(MSG_STOP);

			});

		recognizer->SessionStopped.Connect([this](const SessionEventArgs& e)
			{
				sendNotifyMessage(MSG_STOP);
				m_recognitionEnd.set_value(); // Notify to stop recognition.
			});

		// Starts continuous recognition. Uses StopContinuousRecognitionAsync() to stop recognition.
		recognizer->StartContinuousRecognitionAsync().wait();


		for (; !m_demuxer.isAborted();) {

			ret = m_demuxer(pkt.get());
			if (ret < 0) {
				if (ret == AVERROR_EOF || avio_feof(m_demuxer->pb))
				{
					break;	//no more reading
				}
				else if (m_demuxer->pb && m_demuxer->pb->error)
				{
					break;
				}
				continue;
			}

			if (pkt->stream_index == m_demuxer.getAudioStreamIndex())
			{
				/* send the packet with the compressed data to the decoder */
				ret = m_demuxer.sendPacket(pkt.get());
				if (ret < 0)
					break;

				/* read all the output frames (in general there may be any number of them */
				for (;;)
				{
					ret = m_demuxer.recieveFrame(frame.get());
					if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
						break;

					int data_size = av_samples_get_buffer_size(nullptr, frame->channels,
						frame->nb_samples,
						static_cast<AVSampleFormat>(frame->format), 1);
					// Push a buffer into the stream
					if (data_size > 0)
					{
						uint64_t dec_channel_layout =
							(frame->channel_layout && frame->channels == av_get_channel_layout_nb_channels(frame->channel_layout)) ?
							frame->channel_layout : av_get_default_channel_layout(frame->channels);
						
						const auto& opts = m_demuxer.getSwrOptions();
						const SwrOptions src_options{ dec_channel_layout, frame->sample_rate,
							static_cast<AVSampleFormat>(frame->format) };

						if (opts != src_options) {
							uint8_t* pOut = nullptr;
							const auto resampled_data_size = m_demuxer.convertDstAudio(src_options, &pOut,
								const_cast<const uint8_t**>(frame->extended_data), frame->nb_samples);

							if (resampled_data_size > 0)
							{
								pushStream->Write(pOut, resampled_data_size);
								av_free(pOut);
							}
						}
						else
						{
							pushStream->Write(frame->data[0], data_size);
						}
					}

					av_frame_unref(frame.get());
				}
			}

			av_packet_unref(pkt.get());
		}

		// Close the push stream.
		pushStream->Close();

		// Waits for recognition end.
		m_recognitionEnd.get_future().get();

		// Stops recognition.
		recognizer->StopContinuousRecognitionAsync().get();
	}

	void SpeechDetector::sendNotifyMessage(int what, const char* text /*= nullptr*/) {
		auto msg = std::make_unique<MessageData>();
		msg->msgID = what;
		if (text)
		{
			char* newText = new char[strlen(text) + 1];
			strcpy_s(newText, strlen(text) + 1, text);
			msg->msgText = std::shared_ptr<char>(newText);
		}
		m_MsgQueue.push(std::move(msg));
	}

	bool SpeechDetector::startRecognizeSpeech()
	{
		m_ReadFileThread = std::make_unique<std::thread>(&SpeechDetector::ReadIOThreadProc, this);
		if (m_ReadFileThread)
		{
			return true;
		}
		return false;
	}
}