#include <cstdint>
class BiquadFilter {
	const double& a1, & a2;
	const double& b0, & b1, & b2;
	double l0, l1;

public:
	BiquadFilter(const double* a, const double* b);
	double filter(double x);
	void reset();
};

class FilterIIR {
	int order;
	int cascade;
	BiquadFilter** biquadFilter;
	double* l;
	bool first_input,is_highpass;
	double first_val;
public:
	FilterIIR(const uint64_t[][6], int, bool is_highpass=true);
	double filter(double x);
	void reset();
};

class FilterFIR {

};