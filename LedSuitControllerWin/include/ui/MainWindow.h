#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QTimer> // Include for QTimer
#include <QPushButton>                  
#include "include/ui/SpectrogramView.h"
#include "include/core/AudioPreprocessor.h"
#include "include/ui/LedSuitPictogram.h"
#include "include/ui/PresetManager.h"
#include "include/core/WaypointCompressor.h"
#include "include/core/TcpClient.h"


class AudioPlayer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    void applySuitState(LedSuitPictogram* ledSuit, const SuitState& state);
    void playAudio();
    void pauseAudio();
    void stopAudio();

private:
    QPushButton* playButton;
    QPushButton* pauseButton;
    QPushButton* stopButton;
    QPushButton* fileButton;
    QPushButton* importButton;
    QPushButton* exportButton;
    QPushButton* settingsButton;
    QPushButton* distributeButton;
    QAction* playPauseAction;
    QWidget* leftWidget;
    QWidget* rightWidget;
    void setupToolbar();                                     
    QVBoxLayout* mainLayout;         // Layout for organizing widgets
    QWidget* centralContainer;       // Central widget container
                                     
    AudioPlayer* audioPlayer; // Pointer to the audio player
    QTimer* updateTimer;      // Timer to sync cursor updates
    LedSuitPictogram* ledSuitPictogram; // Pointer to pictogram
    SpectrogramView* spectrogramView;   // Pointer to spectrogram view  
    WaypointCompressor* waypointCompressor;                                        
                              
    PresetManager presetManager;                    // Manages the presets
    std::vector<LedSuitPictogram*> pictograms; 
    std::vector<TcpClient*> tcpClients;

    int numSuits; // Number of suits, loaded from appconfig.json

    void setupPictogramGrid();
    void exportFile();
    void importFile();
    void settings();
    void openFileExplorer();
    void loadTestData();             // Test data for the spectrogram
    void loadMusicFile(const std::string& filePath); 
    void applyPreset(const std::string& presetName);
    void setupPresets();
    void deletePreset(const std::string& presetName);
    void createPresetButtons();
    void applyWaypointState(const std::vector<SuitState>& suitStates);
    void synchronizeAllDevices();

    int loadAppConfig(const QString& configFilePath);
    qint64 tStart = 0;
    void loadSuitConfig(const QString& configFilePath);
    bool lanTimingEnabled = false;
    QScrollArea* scrollArea; // To make the list scrollable
    QVBoxLayout* presetLayout;

private slots:
    void distributeWaypoints();

};



#endif // MAINWINDOW_H

