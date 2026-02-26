#pragma once

/**
 * @class WaveOutDriver
 * @brief SDL2-based audio driver with the same interface as the Windows WaveOut version.
 *
 * This class provides stereo 16-bit PCM audio playback on Linux/Steam Deck
 * using SDL2, presenting the same API as the Windows WaveOut-based version.
 */

#include <SDL2/SDL.h>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cstring>

class WaveOutDriver {
public:
    WaveOutDriver(int sampleRate = 44100, int channels = 2, int bitsPerSample = 16,
        int bufferFrames = 1024, int numBuffers = 4, int ringFrames = 44100)
        : m_deviceId(0)
        , bufferFrames(bufferFrames)
        , numBuffers(numBuffers)
        , sampleRate(sampleRate)
        , channels(channels)
        , bitsPerSample(bitsPerSample)
        , ringSize(ringFrames)
        , ringHead(0)
        , ringTail(0)
    {
        ensureRingSize();
        initDevice();
    }

    ~WaveOutDriver() {
        cleanup();
    }

    // Dummy event handle (returns 0 on Linux - not used)
    intptr_t eventHandle() const { return 0; }

    // Process audio - no-op with SDL2 callback model
    void onAudioEvent() {}

    void pushSample(int16_t left, int16_t right) {
        SDL_LockAudioDevice(m_deviceId);
        size_t nextHead = ringHead + 2;
        if (nextHead >= ringBuffer.size()) nextHead = 0;
        if (nextHead != ringTail) {
            ringBuffer[ringHead] = left;
            ringBuffer[ringHead + 1] = right;
            ringHead = nextHead;
        }
        SDL_UnlockAudioDevice(m_deviceId);
    }

    void resizeBufferFrames(int newFrames) {
        if (newFrames <= 0) return;
        bufferFrames = newFrames;
        ensureRingSize();
        cleanup();
        initDevice();
    }

    void setNumBuffers(int newCount) {
        if (newCount <= 0) return;
        numBuffers = newCount;
        ensureRingSize();
        cleanup();
        initDevice();
    }

    void setSampleRate(int newRate) {
        if (newRate <= 0) return;
        sampleRate = newRate;
        ensureRingSize();
        cleanup();
        initDevice();
    }

    size_t availableSamples() const {
        if (ringHead >= ringTail)
            return ringHead - ringTail;
        else
            return ringBuffer.size() - (ringTail - ringHead);
    }

    size_t availableFrames() const {
        return availableSamples() / 2;
    }

private:
    SDL_AudioDeviceID m_deviceId;
    int bufferFrames;
    int numBuffers;
    int sampleRate;
    int channels;
    int bitsPerSample;

    std::vector<int16_t> ringBuffer;
    size_t ringSize;
    size_t ringHead;
    size_t ringTail;

    void ensureRingSize() {
        size_t minFrames = (size_t)bufferFrames * numBuffers * 2;
        if (ringSize < minFrames) ringSize = minFrames;
        size_t requiredSamples = ringSize * 2;
        if (ringBuffer.size() < requiredSamples) {
            ringBuffer.resize(requiredSamples, 0);
            ringHead = ringTail = 0;
        }
    }

    static void sdlAudioCallback(void* userdata, Uint8* stream, int len) {
        WaveOutDriver* self = static_cast<WaveOutDriver*>(userdata);
        int16_t* out = reinterpret_cast<int16_t*>(stream);
        int frames = len / (self->channels * self->bitsPerSample / 8);

        for (int f = 0; f < frames; f++) {
            if (self->ringTail != self->ringHead) {
                out[f * 2 + 0] = self->ringBuffer[self->ringTail];
                self->ringTail++;
                if (self->ringTail >= self->ringBuffer.size()) self->ringTail = 0;

                out[f * 2 + 1] = self->ringBuffer[self->ringTail];
                self->ringTail++;
                if (self->ringTail >= self->ringBuffer.size()) self->ringTail = 0;
            } else {
                out[f * 2 + 0] = 0;
                out[f * 2 + 1] = 0;
            }
        }
    }

    void initDevice() {
        SDL_AudioSpec desired = {};
        desired.freq = sampleRate;
        desired.format = AUDIO_S16SYS;
        desired.channels = (Uint8)channels;
        desired.samples = (Uint16)bufferFrames;
        desired.callback = sdlAudioCallback;
        desired.userdata = this;

        SDL_AudioSpec obtained = {};
        m_deviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
        if (m_deviceId == 0) {
            throw std::runtime_error(std::string("SDL_OpenAudioDevice failed: ") + SDL_GetError());
        }
        SDL_PauseAudioDevice(m_deviceId, 0); // start playing
    }

    void cleanup() {
        if (m_deviceId != 0) {
            SDL_CloseAudioDevice(m_deviceId);
            m_deviceId = 0;
        }
    }
};
