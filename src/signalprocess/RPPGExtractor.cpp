#include "RPPGExtractor.h"
#include <RPPGExtractor.h>
#include <unsupported/Eigen/FFT>
#include <Eigen/Dense>
#include "OverlapAdding.h"
#include "filter.h"
#include <opencv2/core.hpp>
#include <cstdarg>


namespace RPPGExtractor {
	static const int HEART_ORDER = 10;
	static const uint64_t HEART_COEFF[][6] = {
0x3eebdd875f4a9911,0x3efbdd875f4a9911,0x3eebdd875f4a9911,0x3ff0000000000000,0xbffb5cd9e6bac110,0x3fe8a2813ef9d0de,
0x3ff0000000000000,0x4000000000000000,0x3ff0000000000000,0x3ff0000000000000,0xbffc5517f69bfdfa,0x3fe974360604de0a,
0x3ff0000000000000,0x0,0xbff0000000000000,0x3ff0000000000000,0xbffcef2cb81aa79c,0x3fecbb4e6fc98ed8,
0x3ff0000000000000,0xc000000000000000,0x3ff0000000000000,0x3ff0000000000000,0xbffe36ed8e2b5e5d,0x3fecc4634feb9812,
0x3ff0000000000000,0xc000000000000000,0x3ff0000000000000,0x3ff0000000000000,0xbfff61091e8d3e0e,0x3fef01d8f5be7d61, };

	static std::vector<FilterIIR*> filters;
	static std::vector<Window<float>*> raw_windows;
	static std::vector<Window<float>*> pos_windows;
	static std::vector<Window<float>*> window_pos_s1;
	static std::vector<Window<float>*> window_pos_s2;
	static std::vector <OverlapAdding<float>*> pos_ovadd;

	static std::vector <std::pair<CHANNEL_TYPE,size_t>> channel_types;
	int create_channel(CHANNEL_TYPE channel_type) {
		if (channel_type == NORM_CHANNEL) {
			filters.push_back(new FilterIIR(HEART_COEFF, HEART_ORDER));
			raw_windows.push_back(new Window<float>(INTERP_FS * (60 + rand() % 30), MAX_WINDOWS_LEN));
			channel_types.push_back({ channel_type ,filters.size()-1 });
		}
		else if (channel_type == POS_CHANNEL) {
			pos_windows.push_back(new Window<float>(INTERP_FS * (60 + rand() % 30), POS_WINSIZE));
			window_pos_s1.push_back(new Window<float>(INTERP_FS * (60 + rand() % 30), POS_WINSIZE));
			window_pos_s2.push_back(new Window<float>(INTERP_FS * (60 + rand() % 30), POS_WINSIZE));
			pos_ovadd.push_back(new OverlapAdding<float>(POS_WINSIZE, INTERP_FS * 80));
			channel_types.push_back({ channel_type ,pos_windows.size()-1 });
		}
		return channel_types.size()-1;
	}
	float process_signal(int c, ...) {
		va_list args;
		va_start(args, c);
		float ret=0;
		int channel = channel_types[c].second;
		if (channel_types[c].first == NORM_CHANNEL) {
			float raw_s = filters[channel]->filter(va_arg(args, double)); 
			raw_windows[channel]->shift(raw_s);
			ret= raw_s;
		}
		else if (channel_types[c].first == POS_CHANNEL) {
			float raw_r = va_arg(args, double);
			float raw_g = va_arg(args, double);
			float raw_b = va_arg(args, double);
			window_pos_s1[channel]->shift(raw_g - raw_b);
			window_pos_s2[channel]->shift(raw_g + raw_b - raw_r * 2);

			Eigen::Map<Eigen::Array<float, POS_WINSIZE, 1>>s1(window_pos_s1[channel]->getHead());
			Eigen::Map<Eigen::Array<float, POS_WINSIZE, 1>>s2(window_pos_s2[channel]->getHead());
			float s1_mean = s1.mean();
			float s1_std = std::sqrt((s1 - s1_mean).square().sum() / (POS_WINSIZE - 1));
			float s2_mean = s2.mean();
			float s2_std = std::sqrt((s2 - s2_mean).square().sum() / (POS_WINSIZE - 1)) + 1e-8;
			pos_ovadd[channel]->add_signal((s1 - s1_mean) + s1_std / s2_std * (s2 - s2_mean));
			ret = pos_ovadd[channel]->getArray(POS_WINSIZE+1)[0];
		}
		va_end(args);
		return ret;
	}

	cv::Mat get_signal(int c, int win_len, unsigned ofs_len, unsigned stride)
	{
		int channel = channel_types[c].second;
		cv::Mat sig;
		if (channel_types[c].first == POS_CHANNEL) {
			auto arr = pos_ovadd[channel]->getArray(win_len, ofs_len);
			if(stride>1)
				return cv::Mat(arr.size()/ stride, stride,CV_32F, arr.data()).col(stride-1);
			else
				return cv::Mat(arr.size(), 1, CV_32F, arr.data());
		}
		else if (channel_types[c].first == NORM_CHANNEL) {
			auto arr = raw_windows[channel]->getArray(win_len, ofs_len);
			if (stride > 1)
				return cv::Mat(arr.size() / stride, stride, CV_32F, arr.data()).col(stride - 1);
			else
				return cv::Mat(arr.size(), 1, CV_32F, arr.data());
		}
		return cv::Mat();
	}



	cv::Mat get_spectrum(int c, unsigned ofs_len, int win_len,int *max_idx,float *snr) {
		static Eigen::VectorXf fft_padding(FFT_LENGTH);
		static Eigen::FFT<float> hr_fft(Eigen::FFT<float>::impl_type(), Eigen::FFT<float>::HalfSpectrum || Eigen::FFT<float>::Unscaled);
		static Eigen::VectorXcf fft_out(FFT_LENGTH);
		static Eigen::VectorXf spectrum(FFT_ROI_LENGTH_MAX_LEN);

		Eigen::Map<Eigen::ArrayXf>* sig;

		int channel = channel_types[c].second;
		cv::Mat spec(FFT_ROI_LENGTH, 1, CV_32F);
		Eigen::Map<Eigen::ArrayXf>spec_eigen((float*)(spec.data), FFT_ROI_LENGTH);
		if (channel_types[c].first == POS_CHANNEL )
			sig = new Eigen::Map<Eigen::ArrayXf>(pos_ovadd[channel]->getArray(win_len, ofs_len));
		else if (channel_types[c].first == NORM_CHANNEL)
			sig = new Eigen::Map<Eigen::ArrayXf>(raw_windows[channel]->getArray(win_len, ofs_len));
		else return cv::Mat();
		fft_padding.setZero();
		fft_padding.head(sig->size()) =
			0.5 * (1 - (2 * EIGEN_PI * 
				Eigen::ArrayXf::LinSpaced(sig->size(), 0, sig->size() - 1)
				/(sig->size() - 1)).cos()) * (*sig);
		hr_fft.fwd(fft_out.data(), fft_padding.data(), FFT_LENGTH);
		delete sig;
		if (max_idx != nullptr) {
			spec_eigen = fft_out.segment(FFT_ROI_MIN, FFT_ROI_LENGTH).cwiseAbs();
			spec_eigen.maxCoeff(max_idx);
		}
		if(snr != nullptr){
			const int slob_width = GET_FFT_MAIN_SLOB_WIDTH(win_len);
			spectrum = fft_out.segment(FFT_ROI_LENGTH_MAX_FRONT, FFT_ROI_LENGTH_MAX_LEN).cwiseAbs();
			float power = spectrum.segment(*max_idx + FFT_ROI_LENGTH_MAX_SLOB_WIDTH - slob_width, slob_width * 2+1).sum();
			float noise = spectrum.sum() - power;
			*snr = 20 * logf(power / noise); 
			spec_eigen = spectrum.segment(FFT_ROI_LENGTH_MAX_SLOB_WIDTH, FFT_ROI_LENGTH);
		}
		return spec;
	}

	float get_sqi(int c1, int c2, int win_len, unsigned ofs_len_c1, unsigned ofs_len_c2) {
		Eigen::Map<Eigen::ArrayXf>* sig1;
		if (channel_types[c1].first == POS_CHANNEL)
			sig1 = new Eigen::Map<Eigen::ArrayXf>(pos_ovadd[channel_types[c1].second]->getArray(win_len, ofs_len_c1));
		else
			sig1 = new Eigen::Map<Eigen::ArrayXf>(raw_windows[channel_types[c1].second]->getArray(win_len, ofs_len_c2));
		Eigen::Map<Eigen::ArrayXf>* sig2;
		if (channel_types[c2].first == POS_CHANNEL)
			sig2 = new Eigen::Map<Eigen::ArrayXf>(pos_ovadd[channel_types[c2].second]->getArray(win_len, ofs_len_c1));
		else
			sig2 = new Eigen::Map<Eigen::ArrayXf>(raw_windows[channel_types[c2].second]->getArray(win_len, ofs_len_c2));
		float sqi;
		if (sig1->size() > sig2->size()) {
			sqi = (sig1->tail(sig2->size()) * *sig2).sum();
		}
		else if (sig1->size() < sig2->size()) {
			sqi = (sig2->tail(sig1->size()) * *sig1).sum();
		}else
			sqi = (*sig2 * *sig1).sum();
		delete sig1, sig2;
		return sqi;
	}

	void reset() {
		for (auto f : filters)f->reset();
		for (auto f : raw_windows)f->reset();
		for (auto f : pos_windows)f->reset();
		for (auto f : window_pos_s1)f->reset();
		for (auto f : window_pos_s2)f->reset();
		for (auto f : pos_ovadd)f->reset();
	}
	void reset(int c) {
		auto channel_type = channel_types[c].first;
		auto channel = channel_types[c].second;
		if (channel_type == NORM_CHANNEL) {
			filters[channel]->reset();
			raw_windows[channel]->reset();
		}
		else if (channel_type == POS_CHANNEL) {
			pos_windows[channel]->reset();
			window_pos_s1[channel]->reset();
			window_pos_s2[channel]->reset();
			pos_ovadd[channel]->reset();
		}
	}

};