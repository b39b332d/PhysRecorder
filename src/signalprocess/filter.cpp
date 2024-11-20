#include "filter.h"
BiquadFilter::BiquadFilter(const double* a, const double* b) :
	a1(a[1]), a2(a[2]), b1(b[1]), b2(b[2]), b0(b[0]), l0(0), l1(0) {}
double BiquadFilter::filter(double x) {
	double y = x * b0 + l0;
	l0 = x * b1 - a1 * y + l1;
	l1 = x * b2 - a2 * y;
	return y;
}
void BiquadFilter::reset() {
	l0 = 0;
	l1 = 0;
}


FilterIIR::FilterIIR(unsigned long long w[][6], int order,bool is_highpass)
	: order(order)
	, cascade(order / 2)
	, is_highpass(is_highpass)
{
	first_input = true;
	biquadFilter = new BiquadFilter * [cascade];
	l = new double[cascade]();

	for (int i = 0; i < cascade; i++) {
		biquadFilter[i] = new BiquadFilter(((double*)w[i]) + 3, (double*)w[i]);
	}
}
double FilterIIR::filter(double x) {
	if (is_highpass) {
		if (first_input) {
			first_input = false;
			first_val = x;
			return 0;
		}
		x -= first_val;
	}
	l[0] = biquadFilter[0]->filter(x);
	for (int i = 1; i < cascade; i++) {
		l[i] = biquadFilter[i]->filter(l[i - 1]);
	}
	return l[cascade - 1];
}

void FilterIIR::reset() {
	first_input = true;
	for (int i = 0; i < cascade; i++) {
		biquadFilter[i]->reset();
		l[i] = 0;
	}
}