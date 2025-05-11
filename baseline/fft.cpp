#include <cmath>
#include <fft.h>
#include <fftw3.h>

class DctCalculator::Impl {
public:
    Impl(size_t width, std::vector<double> *input, std::vector<double> *output)
        : width(width), input(input), output(output) {
    }

    void Inverse() {
        for (size_t i = 0; i < width; ++i) {
            input->data()[i] *= kSq2;
            input->data()[i * width] *= kSq2;
        }
        fftw_plan plan = fftw_plan_r2r_2d(width, width, input->data(), output->data(), FFTW_REDFT01,
                                          FFTW_REDFT01, FFTW_ESTIMATE);
        fftw_execute(plan);
        fftw_destroy_plan(plan);
        for (auto &el : *output) {
            el /= 16;
        }
    }

    const double kSq2 = M_SQRT2;
    size_t width;
    std::vector<double> *input;
    std::vector<double> *output;
};

DctCalculator::DctCalculator(size_t width, std::vector<double> *input,
                             std::vector<double> *output) {
    if (width * width != input->size()) {
        throw std::invalid_argument("Invalid input size");
    }
    if (width * width != output->size()) {
        throw std::invalid_argument("Invalid output size");
    }
    impl_ = std::make_unique<Impl>(width, input, output);
}

void DctCalculator::Inverse() {
    impl_->Inverse();
}

DctCalculator::~DctCalculator() = default;
