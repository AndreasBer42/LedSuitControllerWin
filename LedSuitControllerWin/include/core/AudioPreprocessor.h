#ifndef AUDIO_PREPROCESSOR_H
#define AUDIO_PREPROCESSOR_H

#include <vector>
#include <string>

class AudioPreprocessor {
public:
    AudioPreprocessor();
    ~AudioPreprocessor();

    // Load audio file and prepare for processing
    bool loadFile(const std::string& filepath, int sampleRate = 48000, int fftSize = 1024);

    // Generate spectrogram data
    void computeSpectrogram(int maxFrequency = 1000);

    // Get spectrogram data for GUI
    const std::vector<std::vector<float>>& getSpectrogram() const;

    // Map time to spectrogram frame
    size_t getFrameIndexForTime(double timeInSeconds) const;

    // Audio metadata
    double getAudioDuration() const;
    int getFrequencyBins() const;
    int getTimeFrames() const;

private:
    std::vector<float> audioSamples;                  // Raw audio data
    std::vector<std::vector<float>> spectrogramData;  // 2D spectrogram matrix
    double duration;                                  // Duration in seconds
    int sampleRate;                                   // Audio sample rate
    int fftSize;                                      // FFT size
    int hopSize;                                      // Frame hop size (e.g., fftSize / 2)
};

#endif // AUDIO_PREPROCESSOR_H

