#include "Encoder.h"

#define UNICODE
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <iostream>
#include <Encoder.h>
#include <EncoderComp.h>
#include PNG_ENCODER_HEADER
#include JPEG_ENCODER_HEADER
#include <EncoderHUFF.hpp>
#include <EncoderRAW.hpp>
#include <semaphore>
namespace encoder {

	EncoderComp* get_pencoder(int width, int height, PIX_TYPE e_type, int quality, PIX_TYPE d_type) {
		if (d_type == PIX_TYPE_MJPG)
			return new JPEG_ENCODER(width, height, e_type, quality);
		else if (d_type == PIX_TYPE_HFYU)
			return new EncoderHUFF(width, height, e_type, quality);
		else if (d_type == PIX_TYPE_RAW)
			return new EncoderRaw(width, height, e_type, quality);
		else
			return nullptr;
	}
	std::set<PIX_TYPE> get_supported_encoders(PIX_TYPE t) {
		std::set<PIX_TYPE> ts;
		if (JPEG_ENCODER::is_support(t))ts.insert(PIX_TYPE_MJPG);
		if (EncoderHUFF::is_support(t))ts.insert(PIX_TYPE_HFYU);
		if (EncoderRaw::is_support(t))ts.insert(PIX_TYPE_RAW);
		return ts;
	}
	class stream_encoder {
		std::counting_semaphore<CPU_COUNT * 2> out_empty = std::counting_semaphore<CPU_COUNT * 2>(CPU_COUNT * 2);
		std::counting_semaphore<CPU_COUNT * 2> out_valid = std::counting_semaphore<CPU_COUNT * 2>(0);
		std::mutex out_lock;
		std::queue<EncodedFrame*> frames_out;
		std::mutex frame_n_lock;
		std::condition_variable frame_n_cond;
		int next_out_frame_n = 0;
		const char * fourcc;
		EncoderComp* pencoder = nullptr;
	public:
		int next_in_frame_n = 0;

		void* stream_get_info( unsigned int& len) {
			len = pencoder->additional_data_size;
			return pencoder->additional_data;
		}

		stream_encoder(int width,int height, PIX_TYPE e_type, int quality, PIX_TYPE d_type):
			fourcc(fourcc)
		{
			pencoder = get_pencoder(width,  height,  e_type,  quality,  d_type);
			return;
		}
		~stream_encoder() {
			delete pencoder;
		}
		EncodedFrame* encode(RawFrame* rgbData) {
			return pencoder->encode(rgbData);
		}

		inline void push(EncodedFrame* frame, int n) {
			std::unique_lock frame_n_ulock(frame_n_lock);
			while (n != next_out_frame_n) {
				frame_n_cond.wait(frame_n_ulock);
			}
			next_out_frame_n++;
			frame_n_cond.notify_all();
			frame_n_ulock.unlock();

			out_empty.acquire();
			out_lock.lock();
			frames_out.push(frame);
			out_lock.unlock();
			out_valid.release();
		}
		EncodedFrame* pop() {
			out_valid.acquire();
			out_lock.lock();
			auto frame = frames_out.front();
			if (frame == nullptr) {
				out_lock.unlock();
				out_valid.release();
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
		RawFrame* frame_p;
		int frame_n;
	}frame_container;
	

	std::vector<std::thread> thread_pool;
	std::counting_semaphore<CPU_COUNT * 2> in_empty = std::counting_semaphore<CPU_COUNT * 2>(CPU_COUNT * 2);
	std::counting_semaphore<CPU_COUNT * 2> in_valid = std::counting_semaphore<CPU_COUNT * 2>(0);
	std::mutex in_lock;
	std::queue<frame_container*> frames_in;
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
				stream_p->push(nullptr, frame->frame_n);
			}
			else {
				auto frame_out = stream_p->encode(frame->frame_p);
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
	}


	void encode(RawFrame* frame, stream_encoder* stream_p) {
		in_empty.acquire();
		in_lock.lock();
		frames_in.push(new frame_container{ stream_p ,frame,stream_p->next_in_frame_n++});
		in_lock.unlock();
		in_valid.release();
	}


	stream_encoder* add_stream(int width, int height, PIX_TYPE e_type, PIX_TYPE d_type, int quality) {
		std::unique_lock l(init_lock);
		if (!is_stream_encoder_init) {
			std::atexit(encoder_stop);
			encoder_init();
		}
		return new stream_encoder(width, height, e_type, quality, d_type);
	}
	void* stream_get_info(stream_encoder*s, unsigned int& a)
	{
		return s->stream_get_info(a);
	}
	void delete_stream(stream_encoder* stream_p) {
		delete stream_p;
	}
	EncodedFrame* read(stream_encoder* stream_p) {
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
