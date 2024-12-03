#define MKL_VERBOSE 1
#include <RPPGExtractor.h>
#include <cstdlib>
#include <opencv2/core.hpp>
using namespace RPPGExtractor;
int main() {
    create_channel(NORM_CHANNEL);
    create_channel(NORM_CHANNEL);
    create_channel(NORM_CHANNEL);
    create_channel(NORM_CHANNEL);
    std::srand(0);
    reset();
    int aaa = FFT_ROI_LENGTH_MAX_SLOB_WIDTH;
    int bbb = FFT_ROI_LENGTH_MAX_FRONT;
    int ccc = FFT_ROI_LENGTH_MAX_LEN;
    while (true) {

        // Define the range
        float lower = 0.0f, upper = 10.0f;

        // Generate a random float in the range [lower, upper]
        float random_float = lower + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / (upper - lower));

        process_signal(0, random_float);
        process_signal(1, random_float);
        int idx=-1;
        float snr;
        auto v = get_spectrum(0, 0, 10 * INTERP_FS, &idx);
        auto b = get_spectrum(1, 0, 10 * INTERP_FS, &idx, &snr);

        //reset();
    }
}