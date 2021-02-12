#include "SwrContext.h"

namespace VAD {

	SwrConvertContext::SwrConvertContext()
	{

	}

	SwrConvertContext::~SwrConvertContext() {

	}

	void SwrConvertContext::setSwrOptions(SwrOptions dst)
	{
		m_dstSwrOpts = dst;
	}

	bool SwrConvertContext::init(SwrOptions src)
	{
		auto pContext = swr_alloc_set_opts(nullptr, m_dstSwrOpts.ch_layout, m_dstSwrOpts.sample_fmt, 
			m_dstSwrOpts.sample_rate, src.ch_layout, src.sample_fmt, src.sample_rate, 0, nullptr);

		auto free_context = [](SwrContext* p) {
			swr_free(&p);
		};

		m_swrCtx = std::unique_ptr<SwrContext,
			decltype(free_context)>(pContext,
				free_context);

		/* initialize the resampling context */
		if (swr_init(m_swrCtx.get()) < 0) {
			return false;
		}
		return true;
	}

	int SwrConvertContext::convert(uint8_t** out, int out_count,
		const uint8_t** in, int in_count)
	{
		/* convert to destination format */
		return swr_convert(m_swrCtx.get(), out, out_count, in, in_count);
	}

}