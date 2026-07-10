#pragma once

#include "IResampler.hpp"
#include <soxr.h>
#include <stdexcept>

class SoxrResampler : public IResampler {
private:
    soxr_t _soxr;
    int _channels;
    int _bytesPerSample;

public:
    SoxrResampler(double inputRate, double outputRate, int channels, int bitsPerSample) 
        : _channels(channels), _bytesPerSample(bitsPerSample / 8) {
        
        soxr_error_t error;
        soxr_quality_spec_t q_spec = soxr_quality_spec(SOXR_HQ, 0);
        _soxr = soxr_create(inputRate, outputRate, channels, &error, nullptr, &q_spec, nullptr);
        
        if (error) {
            throw std::runtime_error("Failed to initialize soxr resampler");
        }
    }

    ~SoxrResampler() override {
        if (_soxr) soxr_delete(_soxr);
    }

    bool process(const BYTE* inBuffer, UINT64 inBytes, BYTE* outBuffer, UINT64& outBytes, UINT64 outCapacity) override {
        size_t inFrames = inBytes / (_channels * _bytesPerSample);
        size_t outCapacityFrames = outCapacity / (_channels * _bytesPerSample);
        size_t framesRead = 0, framesDone = 0;

        soxr_error_t error = soxr_process(_soxr, 
                                          inBuffer, inFrames, &framesRead, 
                                          outBuffer, outCapacityFrames, &framesDone);
        
        if (error) return false;
        
        outBytes = framesDone * _channels * _bytesPerSample;
        return true;
    }
};
