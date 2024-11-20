#pragma once
#include <functional>
#include <queue>
#include <atomic>
#define PIX_FOURCC_TO_UINT32(str) \
    ((static_cast<uint32_t>(str[0]) << 0) | \
     (static_cast<uint32_t>(sizeof(str)>2?str[1]:0x20) << 8) | \
     (static_cast<uint32_t>(sizeof(str)>3?str[2]:0x20) << 16) | \
     (static_cast<uint32_t>(sizeof(str)>4?str[3]:0x20) << 24))

#define PIX_TYPE_DEFINE_MACRO(val) PIX_TYPE_##val = PIX_FOURCC_TO_UINT32(#val)

typedef enum {
	PIX_TYPE_DEFINE_MACRO(RAW),
	PIX_TYPE_DEFINE_MACRO(RGB8),
	PIX_TYPE_DEFINE_MACRO(BGR8),
	PIX_TYPE_DEFINE_MACRO(RGBA),
	PIX_TYPE_DEFINE_MACRO(BGRA),
	PIX_TYPE_DEFINE_MACRO(ARGB),
	PIX_TYPE_DEFINE_MACRO(RGB5),
	PIX_TYPE_DEFINE_MACRO(BGR5),
	PIX_TYPE_DEFINE_MACRO(RGB6),
	PIX_TYPE_DEFINE_MACRO(BGR6),
	PIX_TYPE_DEFINE_MACRO(L16),
	PIX_TYPE_DEFINE_MACRO(L8),
	PIX_TYPE_DEFINE_MACRO(D16),

	PIX_TYPE_DEFINE_MACRO(I420),
	PIX_TYPE_DEFINE_MACRO(NV12),
	PIX_TYPE_DEFINE_MACRO(NV21),
	PIX_TYPE_DEFINE_MACRO(Y12I),

	PIX_TYPE_DEFINE_MACRO(I422),
	PIX_TYPE_DEFINE_MACRO(UYVY),
	PIX_TYPE_DEFINE_MACRO(YUY2),
	PIX_TYPE_DEFINE_MACRO(YUYV),

	PIX_TYPE_DEFINE_MACRO(RS10),
	PIX_TYPE_DEFINE_MACRO(RS16),
	PIX_TYPE_DEFINE_MACRO(RS8),

	PIX_TYPE_DEFINE_MACRO(MJPG),
	PIX_TYPE_DEFINE_MACRO(MPNG),
	PIX_TYPE_DEFINE_MACRO(HFYU),
	PIX_TYPE_DEFINE_MACRO(FFV1),

	PIX_TYPE_DEFINE_MACRO(IGN),
	PIX_TYPE_DEFINE_MACRO(UNK),
	PIX_TYPE_DEFINE_MACRO(ERR)
} PIX_TYPE;

#define GET_PIX_TYPE_NAME(val) std::string({static_cast<char>((val >> 0) & 0xFF),\
								static_cast<char>((val >> 8) & 0xFF),\
								static_cast<char>((val >> 16) & 0xFF),\
								static_cast<char>((val >> 24) & 0xFF)})


struct Resolution {
	uint32_t width;
	uint32_t height;
};

struct Ratio {
	uint32_t numerator;
	uint32_t denominator;
	float get() { return float(numerator) / denominator; }
};


#define RawFrame_PROFILE_(f) ((capture::CameraProfile*)((f)->profile))
#define RawFrame_WIDTH_(f) RawFrame_PROFILE_(f)->resolution.width
#define RawFrame_HEIGHT_(f) RawFrame_PROFILE_(f)->resolution.height
#define RawFrame_FORMAT_(f) RawFrame_PROFILE_(f)->format
struct RawFrame {
	unsigned char* raw_frame = nullptr;
	unsigned int raw_frame_len;
	long long frame_ts; // microseconds
	void* profile;
	std::atomic<int> ref_cnt=0;
	void* bgr_frame = nullptr;
	void release() {
		if (--ref_cnt <= 0)delete this;
	}
	void acquire() {
		ref_cnt++;
	}
	std::queue<std::function<void()>> free_funcs;
	~RawFrame() {
		while (!free_funcs.empty()) {
			free_funcs.front()();
			free_funcs.pop();
		}
	}
};
typedef std::vector<std::vector<RawFrame*>> FrameSet;

struct EncodedFrame {
	unsigned char* data = nullptr;
	unsigned int size = 0;
	std::queue<std::function<void()>> free_funcs;
	EncodedFrame(unsigned char* data, unsigned int encoded_frame_size, const std::function<void()>& lambda = nullptr) :
		data(data), size(encoded_frame_size)
	{
		if (lambda) {
			free_funcs.push(lambda);
		}
		else {
			free_funcs.push([this]() {free(this->data); });
		}
		
	}
	~EncodedFrame() {
		while (!free_funcs.empty()) {
			free_funcs.front()();
			free_funcs.pop();
		}
	}
};