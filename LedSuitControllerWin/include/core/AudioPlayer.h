
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <QObject>
#include <portaudio.h>
#include <vector>
#include <string>

class AudioPlayer : public QObject {
    Q_OBJECT

public:
    AudioPlayer();
    ~AudioPlayer();

    // Load an audio file
    bool loadFile(const std::string& filepath);

    // Playback controls
    void play();
    void pause();
    void stop();
    void seek(double positionInSeconds);

    // Get playback information
    double getCurrentTime() const;
    double getTotalDuration() const;

    bool isPlaying() const { return isPlaying_; }

    // Waveform data extraction
    std::vector<float> getWaveformData(size_t sampleCount) const;

signals:
    void playbackFinished(); // Signal emitted when playback completes
    void playbackPositionChanged(double currentTime);
                             

private:
    // Internal variables
    PaStream* audioStream;
    std::string audioFilePath;
    double currentTime;       // Current playback time in seconds
    double totalDuration;     // Total audio duration in seconds
    bool isPlaying_;
    // Waveform data storage
    std::vector<float> waveform;

    // Internal helpers
    void extractWaveformData();
};

#endif // AUDIO_PLAYER_H

