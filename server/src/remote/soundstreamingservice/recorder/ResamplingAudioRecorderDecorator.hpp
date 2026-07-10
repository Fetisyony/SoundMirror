#pragma once

#include "IAudioRecorder.hpp"
#include "IResampler.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
#include <cmath>

class ResamplingAudioRecorderDecorator : public IAudioRecorder {
public:
    ResamplingAudioRecorderDecorator(std::shared_ptr<IAudioRecorder> baseRecorder,
                                     std::shared_ptr<IResampler> resampler,
                                     int targetSampleRate)
        : _baseRecorder(std::move(baseRecorder)),
          _resampler(std::move(resampler))
    {
        WAVEFORMATEX* baseFormat = _baseRecorder->getFormat();
        _targetFormat = *baseFormat;
        _targetFormat.nSamplesPerSec = targetSampleRate;
        _targetFormat.nAvgBytesPerSec = targetSampleRate * _targetFormat.nBlockAlign;
    }

    WAVEFORMATEX* getFormat() override { return &_targetFormat; }

    UINT64 getBytesInSecond() const override {
        return _targetFormat.nSamplesPerSec * (_targetFormat.wBitsPerSample / 8) * _targetFormat.nChannels;
    }

    void startRecording() override { _baseRecorder->startRecording(); }

    bool isEnough(UINT64 received, UINT64 bytesInSecond, long double chunkSeconds) override {
        return received >= getBytesInSecond() * chunkSeconds;
    }

    void collectSound(BYTE* destBuffer, UINT64& bytesReceived, UINT64 bufferLimit) override {
        UINT64 baseBytesReceived = 0;

        UINT64 targetFramesCapacity = bufferLimit / _targetFormat.nBlockAlign;
        double ratio = static_cast<double>(_baseRecorder->getFormat()->nSamplesPerSec) / _targetFormat.nSamplesPerSec;

        UINT64 inputFramesNeeded = static_cast<UINT64>(std::ceil(targetFramesCapacity * ratio)) + 10;
        UINT64 requiredInputSize = inputFramesNeeded * _baseRecorder->getFormat()->nBlockAlign;

        if (_tempBuffer.size() < requiredInputSize) {
            _tempBuffer.resize(requiredInputSize);
        }

        _baseRecorder->collectSound(_tempBuffer.data(), baseBytesReceived, _tempBuffer.size());

        if (baseBytesReceived > 0) {
            if (!_resampler->process(_tempBuffer.data(), baseBytesReceived, destBuffer, bytesReceived, bufferLimit)) {
                throw std::runtime_error("High-quality resampling failed during collectSound.");
            }
        } else {
            bytesReceived = 0;
        }
    }

private:
    std::shared_ptr<IAudioRecorder> _baseRecorder;
    std::shared_ptr<IResampler> _resampler;
    WAVEFORMATEX _targetFormat;
    std::vector<BYTE> _tempBuffer;
};
