#ifndef _OVERLAPADDING_H_
#define _OVERLAPADDING_H_

#include <Eigen/Dense>
#include "window.h"

template<class T>
class OverlapAdding {
	Eigen::ArrayX<T> ovadd;
	Eigen::ArrayX<T> ovadd_weight;
	Window<T> signal_window;
	unsigned int size;
	int shift_len, total_len;
	int stride;
public:
	OverlapAdding(int shift_len, int total_len,int stride=1):
		signal_window(total_len * 2, total_len),
		ovadd_weight(shift_len),
		ovadd(shift_len),
		shift_len(shift_len),
		total_len(total_len),
		stride(stride),
		size(0)
	{
		reset();
	}


	template <typename Derived> // Optimized for calculation only once a loop, evaluate every call!
	void add_signal(const Eigen::ArrayBase<Derived>& signal,int stride) {
		if(size < total_len) size += stride;

		ovadd_weight.head(shift_len - stride) = ovadd_weight.tail(shift_len - stride);
		ovadd_weight.tail(stride).setZero();
		ovadd_weight += 1;

		ovadd.head(shift_len - stride) = ovadd.tail(shift_len - stride);
		ovadd.tail(stride).setZero();
		ovadd += signal;
		signal_window.append_zero(stride);
		Eigen::Map<Eigen::ArrayX<T>>(signal_window.getTail() - shift_len, shift_len) =
			ovadd / ovadd_weight;

	}

	template <typename Derived> // Optimized for calculation only once a loop, evaluate every call!
	void add_signal(const Eigen::ArrayBase<Derived>& signal) {
		if (size < shift_len){
			ovadd_weight.head(shift_len - stride) = ovadd_weight.tail(shift_len - stride);
			ovadd_weight.tail(stride).setZero();
			ovadd_weight += 1;
		}
		if (size < total_len) size += stride;

		ovadd.head(shift_len - stride) = ovadd.tail(shift_len - stride);
		ovadd.tail(stride).setZero();
		ovadd += signal;
		signal_window.append_zero(stride);
		Eigen::Map<Eigen::ArrayX<T>>(signal_window.getTail() - shift_len, shift_len) =
			ovadd / ovadd_weight;

	}
	
	Eigen::Map<Eigen::ArrayXf> getArray(int max_size,int tail_ofs=0) {
		max_size = std::min(total_len, max_size);
		int current_len = size + shift_len;
		if (current_len < tail_ofs) {
			return Eigen::Map<Eigen::ArrayX<T>>({0}, 1);
		}
		if (current_len < max_size+ tail_ofs) {
			return Eigen::Map<Eigen::ArrayX<T>>(signal_window.getTail() -
				current_len, current_len -tail_ofs);
		}
		return Eigen::Map<Eigen::ArrayX<T>>(signal_window.getTail() -
			max_size- tail_ofs, max_size);
	}

	void reset() {
		ovadd_weight.setZero();
		ovadd.setZero();
		signal_window.reset();
		size = 0;
	}
};

#endif