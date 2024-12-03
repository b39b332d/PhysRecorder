#ifndef __RPPGEXTRACTOR_H__
#define __RPPGEXTRACTOR_H__

#define INTERP_FS 60.0f
#define UPDATE_RGB_GRAPH_STRIDE 3
#define UPDATE_FFT_STRIDE 6
#define INTERP_CYC double(1.0/INTERP_FS)

#define UPDATE_RGB_GRAPH_STRIDE_SEC (INTERP_CYC*UPDATE_RGB_GRAPH_STRIDE)

#define FFT_RESOLUTION_BPM_EXPECT 1.0f
#define POS_WINSIZE_SEC_EXPECT 1.5f
#define POS_WINSIZE     (int(POS_WINSIZE_SEC_EXPECT*INTERP_FS)/UPDATE_RGB_GRAPH_STRIDE*UPDATE_RGB_GRAPH_STRIDE)
#define POS_WINSIZE_SEC ((float)POS_WINSIZE/INTERP_FS)
#define POS_WIN_ON_RGB_GRAPH (POS_WINSIZE / UPDATE_RGB_GRAPH_STRIDE)

#define FREQ_ROI_MIN_BPM_EXPECT 50.0f
#define FREQ_ROI_MAX_BPM_EXPECT 180.0f
#define FREQ_ROI_RESOLUTION 5

#define FFT_RESOLUTION_EXPECT (FFT_RESOLUTION_BPM_EXPECT/60)
#define FFT_LENGTH int(INTERP_FS/FFT_RESOLUTION_EXPECT)

#define FFT_RESOLUTION ((float)INTERP_FS/FFT_LENGTH)
#define FFT_RESOLUTION_BPM (FFT_RESOLUTION*60)



#define FFT_ROI_MIN int((float)FREQ_ROI_MIN_BPM_EXPECT/60/FFT_RESOLUTION)
#define FFT_ROI_MAX int((float)FREQ_ROI_MAX_BPM_EXPECT/60/FFT_RESOLUTION)

#define FREQ_ROI_MIN_BPM (FFT_RESOLUTION_BPM*FFT_ROI_MIN)
#define FREQ_ROI_MAX_BPM (FFT_RESOLUTION_BPM*FFT_ROI_MAX)

#define GET_FFT_MAIN_SLOB_WIDTH(signal_length)\
		int((float)FFT_LENGTH*8/6.28/(signal_length))

#define FFT_ROI_LENGTH (FFT_ROI_MAX-FFT_ROI_MIN)

#define MIN_WINDOWS_SEC 2
#define MAX_WINDOWS_SEC 20
#define MIN_WINDOWS_LEN int(MIN_WINDOWS_SEC*INTERP_FS)
#define MAX_WINDOWS_LEN int(MAX_WINDOWS_SEC*INTERP_FS)

#define FFT_ROI_LENGTH_MAX_SLOB_WIDTH GET_FFT_MAIN_SLOB_WIDTH(MIN_WINDOWS_LEN)
#define FFT_ROI_LENGTH_MAX_FRONT  (FFT_ROI_MIN-FFT_ROI_LENGTH_MAX_SLOB_WIDTH)
#define FFT_ROI_LENGTH_MAX_LEN int(FFT_ROI_LENGTH+2*FFT_ROI_LENGTH_MAX_SLOB_WIDTH)

namespace cv {
	class Mat;
}
namespace RPPGExtractor {

	typedef enum {
		NORM_CHANNEL,
		POS_CHANNEL
	}CHANNEL_TYPE;
	int create_channel(CHANNEL_TYPE channel_type);
	float process_signal(int c, ...);
	cv::Mat get_signal(int c, int win_len, unsigned ofs_len = 0, unsigned stride=1);
	cv::Mat get_spectrum(int c, unsigned ofs_len, int win_len, int* max_idx=nullptr, float* snr=nullptr);
	float get_sqi(int c1, int c2, int win_len,unsigned  ofs_len_c1 =0, unsigned ofs_len_c2 = 0);
	void reset(int c);
	void reset();
};

#endif