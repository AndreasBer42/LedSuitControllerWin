#include "include/core/AudioPreprocessor.h"
#include "../../include/libsndfile/sndfile.h"   // For audio file handling
#include "../../include/fftw3/fftw3.h"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <qmath.h>

AudioPreprocessor::AudioPreprocessor()
    : duration(0), sampleRate(48000), fftSize(1024), hopSize(512) {}

AudioPreprocessor::~AudioPreprocessor() {}

bool AudioPreprocessor::loadFile(const std::string& filepath, int sampleRate, int fftSize) {
    this->sampleRate = sampleRate;
    this->fftSize = fftSize;
    this->hopSize = fftSize / 2; // Default 50% overlap

    SF_INFO sfinfo;
    SNDFILE* sndfile = sf_open(filepath.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        std::cerr << "Failed to open audio file: " << sf_strerror(sndfile) << std::endl;
        return false;
    }

    // Check if the audio is mono
    if (sfinfo.channels != 1) {
        std::cerr << "Only mono audio is supported for preprocessing. Channels: " << sfinfo.channels << std::endl;
        sf_close(sndfile);
        return false;
    }

    // Read audio samples
    audioSamples.resize(sfinfo.frames);
    sf_read_float(sndfile, audioSamples.data(), sfinfo.frames);
    sf_close(sndfile);

    duration = static_cast<double>(sfinfo.frames) / sfinfo.samplerate;
    std::cout << "Loaded file: " << filepath << ", Duration: " << duration << " seconds, Sample Rate: " << sampleRate << std::endl;

    return true;
}

void AudioPreprocessor::computeSpectrogram(int maxFrequency) {
    size_t numFrames = (audioSamples.size() - fftSize) / hopSize + 1;

    // Map maxFrequency to the corresponding bin
    int maxBin = static_cast<int>((maxFrequency / static_cast<float>(sampleRate / 2.0)) * (fftSize / 2));
    maxBin = std::min(maxBin, fftSize / 2); // Ensure maxBin is within bounds

    // Define the number of bins for the logarithmic scale
    int logBins = std::min(maxBin, 128); // Adjust as necessary for resolution
    spectrogramData.resize(numFrames, std::vector<float>(logBins, 0.0f));

    // Generate logarithmic scale indices
    std::vector<int> logBinIndices(logBins + 1);
    for (int i = 0; i <= logBins; ++i) {
        logBinIndices[i] = static_cast<int>(
            std::pow(10, i / static_cast<float>(logBins) * std::log10(maxBin + 1)) - 1
        );
        logBinIndices[i] = std::min(logBinIndices[i], maxBin);
    }

    std::vector<float> window(fftSize);
    for (size_t i = 0; i < fftSize; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fftSize - 1))); // Hann window
    }

    fftwf_plan plan;
    std::vector<float> inputBuffer(fftSize);
    std::vector<float> outputBuffer(fftSize);
    plan = fftwf_plan_r2r_1d(fftSize, inputBuffer.data(), outputBuffer.data(), FFTW_R2HC, FFTW_ESTIMATE);

    float maxMagnitude = 0.0f;

    // First pass: Compute spectrogram and find max magnitude
    for (size_t frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
        size_t startIdx = frameIdx * hopSize;

        // Apply window function
        for (size_t i = 0; i < fftSize; ++i) {
            inputBuffer[i] = audioSamples[startIdx + i] * window[i];
        }

        // Perform FFT
        fftwf_execute(plan);

        // Compute magnitude spectrum and map to logarithmic bins
        std::vector<float> linearSpectrum(maxBin + 1, 0.0f);
        for (size_t i = 0; i <= maxBin; ++i) {
            linearSpectrum[i] = std::sqrt(outputBuffer[i] * outputBuffer[i]);
        }

        // Aggregate linear spectrum into logarithmic bins
        for (int i = 0; i < logBins; ++i) {
            float sum = 0.0f;
            int count = 0;
            for (int j = logBinIndices[i]; j < logBinIndices[i + 1]; ++j) {
                sum += linearSpectrum[j];
                count++;
            }
            spectrogramData[frameIdx][i] = (count > 0) ? sum / count : 0.0f;
            maxMagnitude = std::max(maxMagnitude, spectrogramData[frameIdx][i]);
        }
    }

    // Second pass: Normalize magnitudes logarithmically
    float logMax = std::log10(1.0f + maxMagnitude);
    for (size_t frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
        for (size_t i = 0; i < logBins; ++i) {
            float magnitude = spectrogramData[frameIdx][i];
            spectrogramData[frameIdx][i] = std::log10(1.0f + magnitude) / logMax; // Normalize
        }
    }

    fftwf_destroy_plan(plan);
    std::cout << "Spectrogram computed with " 
              << spectrogramData.size() << " time frames and " 
              << spectrogramData[0].size() << " frequency bins (logarithmic y-axis)." << std::endl;
}



const std::vector<std::vector<float>>& AudioPreprocessor::getSpectrogram() const {
    return spectrogramData;
}

size_t AudioPreprocessor::getFrameIndexForTime(double timeInSeconds) const {
    return static_cast<size_t>((timeInSeconds / duration) * spectrogramData.size());
}

double AudioPreprocessor::getAudioDuration() const {
    return duration;
}

int AudioPreprocessor::getFrequencyBins() const {
    return fftSize / 2 + 1;
}

int AudioPreprocessor::getTimeFrames() const {
    return spectrogramData.size();
}

