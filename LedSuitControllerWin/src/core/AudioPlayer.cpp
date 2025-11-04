#include "include/core/AudioPlayer.h"
#include "../../include/portaudio/portaudio.h"
#include <iostream>    // For debug messages
#include <stdexcept>   // For exceptions
#include "../../include/libsndfile/sndfile.h"   // For audio file handling
#include <vector>      // For std::vector
#include <string>
#include <chrono>
#include <thread>
#include <QObject>
#include <QTimer>

AudioPlayer::AudioPlayer() : currentTime(0.0), totalDuration(0.0), isPlaying_(false) {
    if (Pa_Initialize() != paNoError) {
        throw std::runtime_error("Failed to initialize PortAudio.");
    }
}


AudioPlayer::~AudioPlayer() {
    if (audioStream) {
        Pa_StopStream(audioStream);
        Pa_CloseStream(audioStream);
    }
    Pa_Terminate();
}


bool AudioPlayer::loadFile(const std::string& filepath) {
    SF_INFO sfinfo = {};
    SNDFILE* sndfile = sf_open(filepath.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        std::cerr << "Failed to open audio file: " << sf_strerror(sndfile) << std::endl;
        return false;
    }

    // Store total duration
    totalDuration = static_cast<double>(sfinfo.frames) / sfinfo.samplerate;

    // Adjust for stereo files
    if (sfinfo.channels == 2) {
        std::cerr << "Downmixing stereo to mono for playback." << std::endl;
        std::vector<float> stereoBuffer(sfinfo.frames * sfinfo.channels);
        sf_read_float(sndfile, stereoBuffer.data(), sfinfo.frames * sfinfo.channels);

        // Downmix to mono
        waveform.resize(sfinfo.frames);
        for (sf_count_t i = 0; i < sfinfo.frames; ++i) {
            waveform[i] = (stereoBuffer[i * 2] + stereoBuffer[i * 2 + 1]) / 2.0f;
        }
    } else if (sfinfo.channels == 1) {
        // Mono audio
        waveform.resize(sfinfo.frames);
        sf_read_float(sndfile, waveform.data(), sfinfo.frames);
    } else {
        std::cerr << "Unsupported number of channels: " << sfinfo.channels << std::endl;
        sf_close(sndfile);
        return false;
    }

    sf_close(sndfile);
    std::cout << "Loaded file: " << filepath << ", Duration: " << totalDuration << " seconds." << std::endl;
    std::cout << "Waveform size: " << waveform.size() << " samples." << std::endl;
    std::cout << "Expected size: " << totalDuration * 48000 << " samples." << std::endl;

    return true;
}




void AudioPlayer::play() {
    if (!isPlaying_) {

        PaError err = Pa_OpenDefaultStream(
            &audioStream,
            0,                           // No input channels
            1,                           // Single output channel (mono)
            paFloat32,                   // 32-bit floating-point audio
            48000,                       // Sample rate (48,000 Hz)
            256,                         // Frames per buffer
            [](const void*, void* outputBuffer, unsigned long framesPerBuffer,
               const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) -> int {
                AudioPlayer* player = static_cast<AudioPlayer*>(userData);
                float* out = static_cast<float*>(outputBuffer);

                unsigned long samplesToProcess = framesPerBuffer;
                if (player->currentTime + samplesToProcess > player->waveform.size()) {
                    samplesToProcess = player->waveform.size() - player->currentTime;
                }

                for (unsigned long i = 0; i < samplesToProcess; ++i) {
                    *out++ = player->waveform[player->currentTime++];
                }

                if (player->currentTime >= player->waveform.size()) {
                    return paComplete;
                }

                return paContinue;
            },
            this
        );

        if (err != paNoError) {
            std::cerr << "Failed to open PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            return;
        }

        err = Pa_StartStream(audioStream);
        if (err != paNoError) {
            std::cerr << "Failed to start PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            return;
        }

        isPlaying_ = true;
        std::cout << "Playing audio..." << std::endl;

        // Monitor playback and emit a signal when it finishes
        QTimer* playbackMonitor = new QTimer(this);
        connect(playbackMonitor, &QTimer::timeout, [this, playbackMonitor]() {
            if (Pa_IsStreamActive(audioStream) == 0) {
                playbackMonitor->stop();
                playbackMonitor->deleteLater();
                emit playbackFinished();
            }
        });

        playbackMonitor->start(100); // Check playback status every 100ms
    }
}






void AudioPlayer::pause() {
    if (isPlaying_ && audioStream) {
        PaError err = Pa_StopStream(audioStream);
        if (err != paNoError) {
            std::cerr << "Failed to pause PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            return;
        }
        isPlaying_ = false;
        std::cout << "Paused audio." << std::endl;
    }
}

void AudioPlayer::stop() {
    if (isPlaying_ || currentTime > 0) {
        if (audioStream) {
            Pa_StopStream(audioStream);
            Pa_CloseStream(audioStream);
            audioStream = nullptr;
        }
        isPlaying_ = false;
        currentTime = 0.0; // Reset current time
        std::cout << "Stopped audio and reset time." << std::endl;
    }
}



void AudioPlayer::seek(double positionInSeconds) {
    if (positionInSeconds >= 0.0 && positionInSeconds <= totalDuration) {
        currentTime = positionInSeconds * 48000; // Convert to samples
        emit playbackPositionChanged(positionInSeconds);
        std::cout << "Seeked to: " << currentTime / 48000 << " seconds." << std::endl;
    } else {
        throw std::out_of_range("Seek position is out of range.");
    }
}

double AudioPlayer::getCurrentTime() const {
    return currentTime / 48000;
}

double AudioPlayer::getTotalDuration() const {
    return totalDuration;
}

std::vector<float> AudioPlayer::getWaveformData(size_t sampleCount) const {
    // Stub: Generate dummy waveform data
    std::vector<float> data(sampleCount, 0.0f);
    for (size_t i = 0; i < sampleCount; ++i) {
        data[i] = static_cast<float>(i % 100) / 100.0f;
    }
    return data;
}

void AudioPlayer::extractWaveformData() {
    // Stub implementation (replace with actual waveform extraction logic)
    waveform = getWaveformData(1000);
    std::cout << "Waveform data extracted." << std::endl;
}

