#pragma once
#include <functional>
#include <queue>
#include <atomic>
#define PIX_FOURCC_TO_UINT32(str) \
    ((static_cast<uint32_t>(str[0])) | \
     (static_cast<uint32_t>(sizeof(str)>2?str[1]:0x20) << 8) | \
     (static_cast<uint32_t>(sizeof(str)>3?str[2]:0x20) << 16) | \
     (static_cast<uint32_t>(sizeof(str)>4?str[3]:0x20) << 24))

#define PIX_TYPE_DEFINE_MACRO(val) PIX_TYPE_##val = PIX_FOURCC_TO_UINT32(#val)
#define IS_PIX_TYPE_RGB(PIX) (PIX==PIX_TYPE_RGB8||PIX==PIX_TYPE_BGR8)
#define IS_PIX_CHAR_TYPE_RGB(PIX) (PIX_FOURCC_TO_UINT32(PIX)==PIX_TYPE_RGB8||PIX_FOURCC_TO_UINT32(PIX)==PIX_TYPE_BGR8)

#define IS_PIX_TYPE_YUV_420(PIX) (PIX==PIX_TYPE_I420||PIX==PIX_TYPE_NV12||PIX==PIX_TYPE_NV21||PIX==PIX_TYPE_Y12I)
#define IS_PIX_TYPE_YUV_422(PIX) (PIX==PIX_TYPE_I422||PIX==PIX_TYPE_UYVY||PIX==PIX_TYPE_YUY2)
#define IS_PIX_TYPE_TOCV(PIX) (PIX==PIX_TYPE_RGB8||PIX==PIX_TYPE_BGR8||PIX==PIX_TYPE_RGBA||PIX==PIX_TYPE_BGRA||PIX==PIX_TYPE_ARGB)
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
	PIX_TYPE_DEFINE_MACRO(Z16),

	// see avi support raw pix types: https://ffmpeg.org/pipermail/ffmpeg-devel/2007-May/035617.html
	PIX_TYPE_DEFINE_MACRO(I420), //yuv420p
	PIX_TYPE_DEFINE_MACRO(NV12), 
	PIX_TYPE_DEFINE_MACRO(NV21),
	PIX_TYPE_DEFINE_MACRO(Y12I),

	PIX_TYPE_DEFINE_MACRO(I422), //yuv422p Y42B
	PIX_TYPE_DEFINE_MACRO(UYVY), //uyvy422
	PIX_TYPE_DEFINE_MACRO(YUY2), //yuyv422
	PIX_TYPE_YUYV = PIX_TYPE_YUY2,

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
inline int get_pix_bit_per_pix(PIX_TYPE p) {
	switch (p) {
	case PIX_TYPE_RGB8: return 24;
	case PIX_TYPE_BGR8: return 24;
	case PIX_TYPE_RGBA: return 32;
	case PIX_TYPE_BGRA: return 32;
	case PIX_TYPE_ARGB: return 32;
	case PIX_TYPE_I420: return 12;
	case PIX_TYPE_NV12: return 12;
	case PIX_TYPE_NV21: return 12;
	case PIX_TYPE_Y12I: return 12;
	case PIX_TYPE_I422: return 16;
	case PIX_TYPE_UYVY: return 16;
	case PIX_TYPE_YUY2: return 16;
	default: return 24;
	}
}
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
#define RawFrame_PROFILE_WIDTH_(f) RawFrame_PROFILE_(f)->resolution.width
#define RawFrame_PROFILE_HEIGHT_(f) RawFrame_PROFILE_(f)->resolution.height
#define RawFrame_PROFILE_FORMAT_(f) RawFrame_PROFILE_(f)->format
#define RawFrame_WIDTH_(f) f->resolution.width
#define RawFrame_HEIGHT_(f) f->resolution.height
#define RawFrame_FORMAT_(f) f->format
struct RawFrame {
	unsigned char* raw_frame = nullptr;
	unsigned int raw_frame_len;
	Resolution resolution;
	PIX_TYPE format;
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
	bool is_empty() {
		return raw_frame == nullptr && bgr_frame == nullptr;
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