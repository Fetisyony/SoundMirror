#include <vector>
#include <memory>
#include <stdexcept>

class MonoDownmixAudioRecorderDecorator : public IAudioRecorder {
public:
    MonoDownmixAudioRecorderDecorator(std::shared_ptr<IAudioRecorder> baseRecorder)
        : _baseRecorder(std::move(baseRecorder))
    {
        WAVEFORMATEX* baseFormat = _baseRecorder->getFormat();

        if (baseFormat->wBitsPerSample != 32 || baseFormat->nChannels != 2) {
            throw std::runtime_error("MonoDownmix expects 32-bit float stereo input.");
        }

        _targetFormat = *baseFormat;

        _targetFormat.nChannels = 1;
        _targetFormat.nBlockAlign = _targetFormat.wBitsPerSample / 8;
        _targetFormat.nAvgBytesPerSec = _targetFormat.nSamplesPerSec * _targetFormat.nBlockAlign;
    }

    WAVEFORMATEX* getFormat() override { return &_targetFormat; }

    UINT64 getBytesInSecond() const override {
        return _targetFormat.nAvgBytesPerSec;
    }

    void startRecording() override { _baseRecorder->startRecording(); }

    bool isEnough(UINT64 received, UINT64 bytesInSecond, long double chunkSeconds) override {
        return received >= getBytesInSecond() * chunkSeconds;
    }

    void collectSound(BYTE* destBuffer, UINT64& bytesReceived, UINT64 bufferLimit) override {
        UINT64 baseBytesReceived = 0;

        UINT64 requiredStereoSize = bufferLimit * 2;
        if (_tempBuffer.size() < requiredStereoSize) {
            _tempBuffer.resize(requiredStereoSize);
        }

        _baseRecorder->collectSound(_tempBuffer.data(), baseBytesReceived, _tempBuffer.size());

        if (baseBytesReceived > 0) {
            UINT64 frames = baseBytesReceived / _baseRecorder->getFormat()->nBlockAlign;

            const float* src = reinterpret_cast<const float*>(_tempBuffer.data());
            float* dst = reinterpret_cast<float*>(destBuffer);

            for (UINT64 i = 0; i < frames; ++i) {
                dst[i] = (src[2 * i] + src[2 * i + 1]) * 0.5f;
            }

            bytesReceived = frames * _targetFormat.nBlockAlign;
        } else {
            bytesReceived = 0;
        }
    }

private:
    std::shared_ptr<IAudioRecorder> _baseRecorder;
    WAVEFORMATEX _targetFormat;
    std::vector<BYTE> _tempBuffer;
};