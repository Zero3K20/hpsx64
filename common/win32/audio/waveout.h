#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <stdexcept>
#include <cstdint>

#pragma comment(lib, "winmm.lib")

/**
 * @class WaveOutDriver
 * @brief A simple wrapper around WinMM waveOut with event-based callbacks and a ring buffer.
 *
 * This class manages playback of stereo 16-bit PCM audio using waveOut with
 * an internal ring buffer. You push samples into the ring buffer, and they
 * are consumed automatically by the waveOut event system.
 *
 * Features:
 *  - Configurable sample rate, buffer size, and number of buffers.
 *  - Dynamically change sample rate, buffer size, and buffer count at runtime.
 *  - Event-based design for use with MsgWaitForMultipleObjectsEx or similar.
 *  - Ensures ring buffer is always large enough (at least 2× playback queue).
 */
class WaveOutDriver {
public:
    /**
     * @brief Construct a new WaveOutDriver
     *
     * @param sampleRate Sample rate in Hz (e.g., 44100).
     * @param channels Number of channels (2 = stereo).
     * @param bitsPerSample Bits per sample (16 typical).
     * @param bufferFrames Frames per buffer (e.g., 1024).
     * @param numBuffers Number of buffers in queue (e.g., 4).
     * @param ringFrames Minimum frames for internal ring buffer (auto-expanded if too small).
     */
    WaveOutDriver(int sampleRate = 44100, int channels = 2, int bitsPerSample = 16,
        int bufferFrames = 1024, int numBuffers = 4, int ringFrames = 44100)
        : hWaveOut(NULL), hEvent(NULL),
        bufferFrames(bufferFrames), numBuffers(numBuffers),
        sampleRate(sampleRate), channels(channels), bitsPerSample(bitsPerSample),
        ringSize(ringFrames), ringHead(0), ringTail(0)
    {
        ensureRingSize();
        initDevice();
        allocateBuffers();
    }

    /**
     * @brief Destroy the WaveOutDriver and release resources.
     */
    ~WaveOutDriver() {
        cleanup();
    }

    /**
     * @brief Get the event handle used for signaling audio buffer completion.
     *
     * This handle can be used in MsgWaitForMultipleObjectsEx or similar
     * wait functions.
     *
     * @return HANDLE The event handle.
     */
    HANDLE eventHandle() const { return hEvent; }

    /**
     * @brief Process audio event: refill any completed buffers.
     *
     * Call this whenever the event handle is signaled.
     */
    void onAudioEvent() {
        for (int i = 0; i < numBuffers; i++) {
            WAVEHDR& hdr = headers[i];
            if (hdr.dwFlags & WHDR_DONE) {
                waveOutUnprepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
                fillBuffer(i);
                waveOutPrepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
                waveOutWrite(hWaveOut, &hdr, sizeof(WAVEHDR));
            }
        }
    }

    /**
     * @brief Push a single stereo frame (L/R) into the ring buffer.
     *
     * @param left Left channel sample.
     * @param right Right channel sample.
     */
    void pushSample(int16_t left, int16_t right) {
        size_t nextHead = ringHead + 2;
        if (nextHead >= ringBuffer.size()) nextHead = 0;

        if (nextHead == ringTail) return; // full, drop

        ringBuffer[ringHead] = left;
        ringBuffer[ringHead + 1] = right;
        ringHead = nextHead;
    }

    /**
     * @brief Dynamically resize buffer size (frames per buffer).
     *
     * @param newFrames New buffer frame size.
     */
    void resizeBufferFrames(int newFrames) {
        if (newFrames <= 0) return;
        bufferFrames = newFrames;
        ensureRingSize();

        waveOutReset(hWaveOut);
        freeBuffers();
        allocateBuffers();
    }

    /**
     * @brief Dynamically change number of buffers in flight.
     *
     * @param newCount New buffer count.
     */
    void setNumBuffers(int newCount) {
        if (newCount <= 0) return;
        numBuffers = newCount;
        ensureRingSize();

        waveOutReset(hWaveOut);
        freeBuffers();
        allocateBuffers();
    }

    /**
     * @brief Dynamically change sample rate (reopens device).
     *
     * @param newRate New sample rate in Hz.
     */
    void setSampleRate(int newRate) {
        if (newRate <= 0) return;
        sampleRate = newRate;
        ensureRingSize();

        waveOutReset(hWaveOut);
        freeBuffers();
        waveOutClose(hWaveOut);

        initDevice();
        allocateBuffers();
    }

    /**
     * @brief Get the number of queued 16-bit samples in the ring buffer.
     *
     * @return size_t Number of samples (not frames).
     */
    size_t availableSamples() const {
        if (ringHead >= ringTail)
            return ringHead - ringTail;
        else
            return ringBuffer.size() - (ringTail - ringHead);
    }

    /**
     * @brief Get the number of queued stereo frames in the ring buffer.
     *
     * @return size_t Number of frames.
     */
    size_t availableFrames() const {
        return availableSamples() / 2;
    }

private:
    HWAVEOUT hWaveOut;   ///< Handle to waveOut device
    HANDLE hEvent;       ///< Event handle for callback
    int bufferFrames;    ///< Frames per buffer
    int numBuffers;      ///< Number of buffers
    int sampleRate;      ///< Sample rate in Hz
    int channels;        ///< Number of channels
    int bitsPerSample;   ///< Bits per sample

    std::vector<int16_t> ringBuffer; ///< Internal ring buffer
    size_t ringSize;                 ///< Requested ring buffer size (frames)
    size_t ringHead;                 ///< Write position in ring buffer
    size_t ringTail;                 ///< Read position in ring buffer

    std::vector<std::vector<uint8_t>> buffers; ///< WaveOut buffers
    std::vector<WAVEHDR> headers;              ///< WaveOut headers

    /**
     * @brief Ensure ring buffer is large enough for current configuration.
     */
    void ensureRingSize() {
        size_t minFrames = bufferFrames * numBuffers * 2;
        if (ringSize < minFrames) ringSize = minFrames;

        size_t requiredSamples = ringSize * 2; // stereo
        if (ringBuffer.size() < requiredSamples) {
            ringBuffer.resize(requiredSamples, 0);
            ringHead = ringTail = 0;
        }
    }

    /**
     * @brief Initialize the waveOut device.
     */
    void initDevice() {
        hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!hEvent)
            throw std::runtime_error("Failed to create event");

        WAVEFORMATEX wfx = {};
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = channels;
        wfx.nSamplesPerSec = sampleRate;
        wfx.wBitsPerSample = bitsPerSample;
        wfx.nBlockAlign = (channels * bitsPerSample) / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        MMRESULT res = waveOutOpen(
            &hWaveOut,
            WAVE_MAPPER,
            &wfx,
            (DWORD_PTR)hEvent,
            0,
            CALLBACK_EVENT
        );
        if (res != MMSYSERR_NOERROR)
            throw std::runtime_error("waveOutOpen failed");
    }

    /**
     * @brief Allocate and queue audio buffers.
     */
    void allocateBuffers() {
        size_t bufferBytes = bufferFrames * (channels * bitsPerSample / 8);
        buffers.resize(numBuffers);
        headers.resize(numBuffers);

        for (int i = 0; i < numBuffers; i++) {
            buffers[i].resize(bufferBytes, 0);
            WAVEHDR& hdr = headers[i];
            ZeroMemory(&hdr, sizeof(WAVEHDR));
            hdr.lpData = reinterpret_cast<LPSTR>(buffers[i].data());
            hdr.dwBufferLength = (DWORD)bufferBytes;
            waveOutPrepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
            waveOutWrite(hWaveOut, &hdr, sizeof(WAVEHDR));
        }
    }

    /**
     * @brief Free allocated buffers.
     */
    void freeBuffers() {
        for (auto& hdr : headers) {
            if (hdr.lpData)
                waveOutUnprepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
        }
        buffers.clear();
        headers.clear();
    }

    /**
     * @brief Cleanup all resources.
     */
    void cleanup() {
        if (hWaveOut) {
            waveOutReset(hWaveOut);
            freeBuffers();
            waveOutClose(hWaveOut);
            hWaveOut = NULL;
        }
        if (hEvent) {
            CloseHandle(hEvent);
            hEvent = NULL;
        }
    }

    /**
     * @brief Fill one buffer with samples from the ring buffer.
     *
     * @param idx Index of the buffer to fill.
     */
    void fillBuffer(int idx) {
        auto& buf = buffers[idx];
        int16_t* data = reinterpret_cast<int16_t*>(buf.data());
        size_t frames = bufferFrames;

        for (size_t f = 0; f < frames; f++) {
            if (ringTail != ringHead) {
                data[f * 2 + 0] = ringBuffer[ringTail];
                ringTail++;
                if (ringTail >= ringBuffer.size()) ringTail = 0;

                data[f * 2 + 1] = ringBuffer[ringTail];
                ringTail++;
                if (ringTail >= ringBuffer.size()) ringTail = 0;
            }
            else {
                data[f * 2 + 0] = 0;
                data[f * 2 + 1] = 0;
            }
        }
    }
};
