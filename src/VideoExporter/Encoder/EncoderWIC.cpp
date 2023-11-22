
#define UNICODE
#include <thread>
#include <vector>
#include <semaphore>
#include <queue>
#include <mutex>
#include <wincodecsdk.h>
#include <Windows.h>
#include <iostream>
#include <Encoder.h>

namespace encoder {

	class stream_encoder {
		std::counting_semaphore<CPU_COUNT * 2> out_empty = std::counting_semaphore<CPU_COUNT * 2>(CPU_COUNT * 2);
		std::counting_semaphore<CPU_COUNT * 2> out_valid = std::counting_semaphore<CPU_COUNT * 2>(0);
		std::mutex out_lock;
		std::queue<std::vector<uint8_t>> frames_out;
		std::mutex frame_n_lock;
		std::condition_variable frame_n_cond;
		int next_out_frame_n = 0;

	public:
		int width;
		int height;
		float quality;
		PIX_TYPE e_type;
		int next_in_frame_n = 0;
		stream_encoder(int width,int height, PIX_TYPE e_type, float quality) :
			width(width),
			height(height),
			quality(quality),
			e_type(e_type)
		{

		}

		inline void push(const std::vector<uint8_t>& frame, int n) {
			std::unique_lock frame_n_ulock(frame_n_lock);
			while (n != next_out_frame_n) {
				frame_n_cond.wait(frame_n_ulock);
			}
			next_out_frame_n++;
			frame_n_ulock.unlock();
			frame_n_cond.notify_all();

			out_empty.acquire();
			out_lock.lock();
			frames_out.push(frame);
			out_lock.unlock();
			out_valid.release();
		}
		std::vector<uint8_t> pop() {
			out_valid.acquire();
			out_lock.lock();
			auto frame = frames_out.front();
			if (frame.size() == 0) {
				out_lock.unlock();
				out_valid.release();
				delete this; // self destory 
			}
			else {
				frames_out.pop();
				out_lock.unlock();
				out_empty.release();
			}
			//std::cout << "b" << frames_out.size() << std::endl;
			return frame;
		}
	};

	typedef struct {
		stream_encoder* stream;
		std::shared_ptr<void> frame_p;
		int frame_n;
	}frame_container;
	

	std::vector<std::thread> thread_pool;
	std::counting_semaphore<CPU_COUNT * 2> in_empty = std::counting_semaphore<CPU_COUNT * 2>(CPU_COUNT * 2);
	std::counting_semaphore<CPU_COUNT * 2> in_valid = std::counting_semaphore<CPU_COUNT * 2>(0);
	std::mutex in_lock;
	std::queue<frame_container*> frames_in;

	IWICImagingFactory* pFactory = nullptr;
	std::mutex lock;
	
	inline std::vector<unsigned char> encode_jpeg(void* rgbData, int width, int height, PIX_TYPE e_type,float quality)
	{

		// Create memory stream for JPEG output
		IStream* pStream = nullptr;
		CreateStreamOnHGlobal(nullptr, TRUE, &pStream);

		// Create JPEG encoder
		IWICBitmapEncoder* pEncoder = nullptr;
		lock.lock();
		pFactory->CreateEncoder(GUID_ContainerFormatJpeg, nullptr, &pEncoder);
		lock.unlock();
		pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);

		// Create JPEG frame encoder
		IWICBitmapFrameEncode* pFrameEncode = nullptr;
		pEncoder->CreateNewFrame(&pFrameEncode, nullptr);
		pFrameEncode->Initialize(nullptr);

		// Set encoder properties
		pFrameEncode->SetSize(width, height);
		WICPixelFormatGUID pixelFormat;
		switch (e_type)
		{
		PIX_TYPE_RGB888:
			pixelFormat = { GUID_WICPixelFormat24bppRGB };
			break;
		PIX_TYPE_GRAY08:
			pixelFormat = { GUID_WICPixelFormat8bppGray };
			break;
		PIX_TYPE_GRAY16:
			pixelFormat = { GUID_WICPixelFormat16bppGray };
			break;
		PIX_TYPE_RGBA32:
			pixelFormat = { GUID_WICPixelFormat32bppRGBA };
			break;
		PIX_TYPE_BGRA32:
			pixelFormat = { GUID_WICPixelFormat32bppBGRA };
			break;
		default:
			pixelFormat = { GUID_WICPixelFormat24bppBGR };
			break;
		}			
		pFrameEncode->SetPixelFormat(&pixelFormat);

		if (quality >= 0) {
			IPropertyBag2* pPropertyBag = nullptr;
			pFrameEncode->QueryInterface(IID_PPV_ARGS(&pPropertyBag));
			PROPBAG2 propBag = { 0, };
			if (quality > 1) {
				propBag.pstrName = const_cast <wchar_t*>(L"Lossless");
				VARIANT varValue;
				varValue.vt = VT_BOOL;
				varValue.boolVal = VARIANT_TRUE;
				pPropertyBag->Write(1, &propBag, &varValue);
			}
			else {
				propBag.pstrName = const_cast <wchar_t*>(L"ImageQuality");
				VARIANT varValue;
				varValue.vt = VT_R4;
				varValue.fltVal = static_cast<float>(quality) / 100.0f;
				pPropertyBag->Write(1, &propBag, &varValue);
			}
			pPropertyBag->Release();
		}

		pFrameEncode->WritePixels(height, width * 3, height* width * 3, reinterpret_cast<BYTE*>(const_cast<void*>(rgbData)));


		// Commit frame and encoder
		pFrameEncode->Commit();
		pEncoder->Commit();

		// Get JPEG data from memory stream
		STATSTG stat;
		pStream->Stat(&stat, STATFLAG_NONAME);
		ULONG jpegSize = static_cast<ULONG>(stat.cbSize.QuadPart);

		// Reset memory stream position
		LARGE_INTEGER zero = { 0 };
		pStream->Seek(zero, STREAM_SEEK_SET, nullptr);

		// Allocate buffer for JPEG data
		std::vector<unsigned char> jpegBuffer(jpegSize);

		// Read JPEG data into buffer
		pStream->Read(jpegBuffer.data(), jpegSize, nullptr);

		// Release resources
		pFrameEncode->Release();
		pEncoder->Release();
		pStream->Release();
		return jpegBuffer;
	}

	void enc_thread() {
		while (true) {
			in_valid.acquire();
			in_lock.lock();
			auto frame = frames_in.front();
			if (frame == nullptr) {
				in_lock.unlock();
				in_valid.release();
				return;
			}
			else {
				frames_in.pop();
				in_lock.unlock();
				in_empty.release();
			}

			auto stream_p = frame->stream;
			if (frame->frame_p == nullptr) {
				stream_p->push({}, frame->frame_n);
			}
			else {
				auto frame_out = encode_jpeg(frame->frame_p.get(), stream_p->width, stream_p->height,stream_p->e_type,stream_p->quality);
				stream_p->push(frame_out, frame->frame_n);

			}
			delete frame;
		}
	}

	std::mutex init_lock;
	bool is_stream_encoder_init = false;

	void encoder_init() {
		for (int i = 0; i < CPU_COUNT; i++) {
			thread_pool.emplace_back(enc_thread);
		}

		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
		is_stream_encoder_init = true; // never parallel
	}


	void encoder_stop() {
		in_empty.acquire();
		in_lock.lock();
		frames_in.push(nullptr);
		in_lock.unlock();
		in_valid.release();
		for (auto& th : thread_pool) {
			th.join();
		}
		pFactory->Release();
	}

	void encode(std::shared_ptr<void> frame, stream_encoder* stream_p) {
		in_empty.acquire();
		in_lock.lock();
		frames_in.push(new frame_container{ stream_p ,frame,stream_p->next_in_frame_n++});
		in_lock.unlock();
		in_valid.release();
	}


	stream_encoder* add_stream(int width, int height, PIX_TYPE e_type,float quality) {
		std::unique_lock l(init_lock);
		if (!is_stream_encoder_init) {
			std::atexit(encoder_stop);
			encoder_init();
		}
		return new stream_encoder(width, height, e_type, quality);
	}
	std::vector<unsigned char> read(stream_encoder* stream_p) {
		return stream_p->pop();
	}

	void stream_close_later(stream_encoder* stream_p) { // delete later
		in_empty.acquire();
		in_lock.lock();
		frames_in.push(new frame_container{ (stream_encoder*)stream_p ,nullptr,stream_p->next_in_frame_n++ });
		in_lock.unlock();
		in_valid.release();
	}

};
