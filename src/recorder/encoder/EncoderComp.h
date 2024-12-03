#pragma once
#include <mutex>
#include <list>
#include <Encoder.h>
#include <frame_types.h>
namespace encoder {
	class EncoderComp {
	public:
		int width, height;
		PIX_TYPE e_type;
		int quality;
		int comp;
		int data_size;
		void* additional_data=nullptr;
		unsigned int additional_data_size=0;
		EncoderComp(int width, int height, PIX_TYPE e_type, int quality) :width(width), height(height), e_type(e_type), quality(quality) {
			if (e_type == PIX_TYPE_RGB8 || e_type == PIX_TYPE_BGR8) {
				comp = 3;
			}
			else if (e_type == PIX_TYPE_BGRA || e_type == PIX_TYPE_RGBA) {
				comp = 4;
			}
			else if (e_type == PIX_TYPE_L8) {
				comp = 1;
			}
			else if (e_type == PIX_TYPE_L16)
				comp = 2;
			data_size = width * height * comp;
		}
		virtual ~EncoderComp() {};
		virtual EncodedFrame* encode(RawFrame* rgbData) = 0;

		static bool is_support(PIX_TYPE e_type) {
			return false;
		}
	};
}