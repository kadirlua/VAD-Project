#pragma once
#include <memory>
#include <future>
#include "Demuxer.h"
#include "Queue.h"

namespace VAD {

	struct MessageData {
		int msgID{};
		std::shared_ptr<char> msgText;

		MessageData() = default;
		~MessageData() = default;
	};

	enum MessageID {
		NO_MSG = 0,
		MSG_PUSH = 100,
		MSG_STOP
	};

	class SpeechDetector {

		static constexpr auto MSG_QUEUE_SIZE = 16;

		using MessageQueue_t = Queue<std::unique_ptr<MessageData>>;
		using fp_MessageLoopCallBack_t = std::function<int(void*)>;
	public:
		SpeechDetector();
		~SpeechDetector();
		SpeechDetector(const SpeechDetector& r) = delete;
		SpeechDetector& operator=(const SpeechDetector& r) = delete;

		bool createVAD(const std::string& filePath, fp_Demuxer_Callback_t fn, void* userdata);
		
		bool startRecognizeSpeech();

		bool createMessageQueue(const fp_MessageLoopCallBack_t& msg_loop);

		MessageQueue_t& getMessageQueue() {
			return m_MsgQueue;
		}

		void sendNotifyMessage(int what, const char* text = nullptr);

		void ReadIOThreadProc();

	private:
		// promise for synchronization of recognition end.
		std::promise<void> m_recognitionEnd;
		MessageQueue_t m_MsgQueue;
		std::unique_ptr<std::thread> m_MessageQueueThread;
		std::unique_ptr<std::thread> m_ReadFileThread;
		Demuxer m_demuxer;
	};
}