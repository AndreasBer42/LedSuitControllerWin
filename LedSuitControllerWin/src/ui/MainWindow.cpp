#include "include/ui/MainWindow.h"
#include "include/core/AudioPlayer.h"
#include "include/ui/Preset.h"
#include "include/ui/PresetManager.h"
#include "include/core/SuitState.h"
#include "include/core/WaypointSerializer.h"
#include "include/core/WaypointCompressor.h"
#include "include/ui/SettingsDialog.h"
#include "include/ConfigUtils.h"
#include <vector>
#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <iostream> 
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QToolBar>
#include <QPixmap>
#include <QDebug>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QIODevice>
#include <QFileDialog>
#include <QMessageBox>
#include <QtNetwork/QUdpSocket>

 

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      centralContainer(new QWidget(this)),
      spectrogramView(new SpectrogramView(this)),
      leftWidget(new QWidget(this)), // Left widget for 1:5 split
      rightWidget(new QWidget(this)), // Right widget for 1:5 split
      audioPlayer(new AudioPlayer()),
      updateTimer(new QTimer(this)), // Allocate the timer
      waypointCompressor(new WaypointCompressor(spectrogramView)) {
    
    ensureConfigFiles(); // Ensure config files are created first
    
    numSuits = loadAppConfig(QDir::currentPath() + "/config/appconfig.json");
    loadSuitConfig(QDir::currentPath() + "/config/suits.json");

    resize(1920, 1080); // Set an adequate initial window size

    setupToolbar();

    // Initialize layout and widgets
    setCentralWidget(centralContainer);

    mainLayout = new QVBoxLayout(centralContainer);

    QHBoxLayout* splitLayout = new QHBoxLayout();

    // Configure left and right widgets
    leftWidget->setStyleSheet("background-color: lightgray;"); // Optional styling
    rightWidget->setStyleSheet("background-color: lightgray;"); // Optional styling

    // Add widgets to the horizontal layout with 1:5 ratio
    splitLayout->addWidget(leftWidget, 1); // 1 part
    splitLayout->addWidget(rightWidget, 5); // 5 parts

    mainLayout->addLayout(splitLayout, 2);

    mainLayout->addWidget(spectrogramView, 1);

    // Prompt the user to select a music file
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open Music File"),
        QDir::homePath(),
        tr("Audio Files (*.wav *.mp3 *.flac)")
    );

    if (filePath.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("No music file selected. Exiting."));
        QTimer::singleShot(0, this, &QWidget::close); // Exit the application
        return;
    }

    // Load the audio file into the spectrogram
    loadMusicFile(filePath.toStdString());

    // Load the audio file into the player
    if (!audioPlayer->loadFile(filePath.toStdString())) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to load the selected audio file."));
        QTimer::singleShot(0, this, &QWidget::close); // Exit the application
        return;
    } else {
        std::cout << "Audio file successfully loaded into AudioPlayer." << std::endl;
    }

    // Connect the AudioPlayer to the SpectrogramView
    spectrogramView->connectAudioPlayer(audioPlayer);
    std::cout << "AudioPlayer connected to SpectrogramView." << std::endl;

    // Connect the SpectrogramView's updatePictograms signal to MainWindow
    connect(spectrogramView, &SpectrogramView::updatePictograms, this, &MainWindow::applyWaypointState);

    // Add LED Suit Pictograms
    setupPictogramGrid();
    setupPresets();
}


int MainWindow::loadAppConfig(const QString& configFilePath) {
    QFile file(configFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open app config file:" << configFilePath;
        return 8; // Default
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "Invalid app config file format.";
        return 8; // Default
    }

    QJsonObject obj = doc.object();
    return obj.contains("numSuits") && obj["numSuits"].isDouble() ? obj["numSuits"].toInt() : 8;
}


std::vector<QString> suitNames;
std::vector<QColor> suitSecondaryColors;
std::vector<std::pair<QString, quint16>> suitConnections; // Vector to hold IPs and ports

void MainWindow::loadSuitConfig(const QString& configFilePath) {
    QFile file(configFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to load suit config file:" << configFilePath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "Invalid suit config file format.";
        return;
    }

    QJsonArray array = doc.array();
    suitNames.clear();
    suitSecondaryColors.clear();
    suitConnections.clear(); // Clear the vector before reloading

    const quint16 defaultPort = 12345; // Define a default port for all devices

    for (const QJsonValue& value : array) {
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            suitNames.push_back(obj["name"].toString());

            // Parse the secondary color (red or green)
            QJsonObject color = obj["color"].toObject();
            bool isGreen = color["green"].toBool();
            suitSecondaryColors.push_back(isGreen ? QColor("green") : QColor("red"));

            // Parse the IP address
            QString ip = obj["ip"].toString();
            if (ip.isEmpty()) {
                qWarning() << "Missing IP for suit:" << obj["name"].toString();
                continue;
            }

            // Add the IP with the default port to the connections vector
            suitConnections.emplace_back(ip, defaultPort);
        }
    }

    qDebug() << "Loaded suit connections:" << suitConnections;
}


uint32_t tStart = 0;

void MainWindow::setupToolbar() {

    // Load icons
    QPixmap playPixmap(":/icons/Play.png");
    QPixmap pausePixmap(":/icons/Pause.png");
    QPixmap stopPixmap(":/icons/Stop.png");
    QPixmap filePixmap(":/icons/File.png");
    QPixmap importPixmap(":/icons/Import.png");
    QPixmap exportPixmap(":/icons/Export.png");
    QPixmap settingsPixmap(":/icons/Settings.png");
    QPixmap templatePixmap(":/icons/Template.png");
    QPixmap addWaypointPixmap(":/icons/AddWaypoint.png");
    QPixmap distributePixmap(":/icons/Distribute.png");
    QPixmap lanPixmap(":/icons/lan.png");
    QPixmap lanGreenPixmap(":/icons/lanGreen.png");



    QIcon playIcon(playPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon pauseIcon(pausePixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon stopIcon(stopPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon fileIcon(filePixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon importIcon(importPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon exportIcon(exportPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon settingsIcon(settingsPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon templateIcon(templatePixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon addWaypointIcon(addWaypointPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon distributeIcon(distributePixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon lanIcon(lanPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    QIcon lanGreenIcon(lanGreenPixmap.scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation));



    // Create play/pause and stop actions
    playPauseAction = new QAction(playIcon, "Play", this);
    QAction* stopAction = new QAction(stopIcon, "Stop", this);
    QAction* fileAction = new QAction(fileIcon, "Files", this);
    QAction* settingsAction = new QAction(settingsIcon, "Settings", this);
    QAction* importAction = new QAction(importIcon, "Import File", this);
    QAction* exportAction = new QAction(exportIcon, "Export File", this);
    QAction* templateAction = new QAction(templateIcon, "Save to Template", this);
    QAction* addWaypointAction = new QAction(addWaypointIcon, "Add Waypoint", this);
    QAction* distributeAction = new QAction(distributeIcon, "Send to Suits", this);
    QAction* lanTimingAction = new QAction(lanIcon, "LAN Timing Off", this);


    // Toggle LAN timing state and update icon and text
    connect(lanTimingAction, &QAction::triggered, this, [this, lanTimingAction, lanIcon, lanGreenIcon]() {
        lanTimingEnabled = !lanTimingEnabled;
        lanTimingAction->setIcon(lanTimingEnabled ? lanGreenIcon : lanIcon);
        lanTimingAction->setText(lanTimingEnabled ? "LAN Timing On" : "LAN Timing Off");
    });


    connect(playPauseAction, &QAction::triggered, this, [this, playIcon, pauseIcon]() {
        if (!audioPlayer->isPlaying()) {
            if (lanTimingEnabled) {
                // Automatically synchronize devices
                try {
                    synchronizeAllDevices();
                } catch (const std::exception& e) {
                    QMessageBox::critical(this, "Error", QString("Failed to synchronize devices: %1").arg(e.what()));
                    return; // Abort playback if synchronization fails
                  }

                  // Calculate delay to T_start
                  const auto currentTime = std::chrono::system_clock::now();
                  const auto unixTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                              currentTime.time_since_epoch())
                                              .count();

                  // Debug: Print current time and target time
                  qWarning() << "Current Unix Time (ms):" << unixTimeMs;
                  qWarning() << "T_Start (ms):" << tStart;

                  const auto delayMs = 0;

                  // Debug: Print calculated delay
                  qWarning() << "Calculated Delay (ms):" << delayMs;

                  if (delayMs >= 0) {
                      QTimer::singleShot(delayMs, this, [this, pauseIcon]() {
                          playAudio();
                          playPauseAction->setIcon(pauseIcon);
                          playPauseAction->setText("Pause");
                      });
                  } else {
                      QMessageBox::warning(this, "Warning", "Calculated delay is negative. Starting playback immediately.");
                      playAudio();
                      playPauseAction->setIcon(pauseIcon);
                      playPauseAction->setText("Pause");
                  }
              } else {
                  // Start playback immediately
                  playAudio();
                  playPauseAction->setIcon(pauseIcon);
                  playPauseAction->setText("Pause");
              }
          } else {
              // Pause playback
              pauseAudio();
              playPauseAction->setIcon(playIcon);
              playPauseAction->setText("Play");
          }
      });




      connect(addWaypointAction, &QAction::triggered, this, [this]() {
          if (!spectrogramView || !audioPlayer) {
              std::cerr << "Error: Missing required components." << std::endl;
              return;
          }

        // Collect the current states of all pictograms
        std::vector<SuitState> currentStates;
        for (LedSuitPictogram* pictogram : pictograms) {
            if (pictogram) {
                currentStates.push_back(pictogram->getCurrentState());
            }
        }

        // Ensure we have states for all suits before proceeding
        if (currentStates.empty()) {
            std::cerr << "Error: No pictograms available to retrieve states." << std::endl;
            return;
        }

        // Get the current playback time from the audio player
        double currentTime = audioPlayer->getCurrentTime();

        // Create a waypoint
        Waypoint waypoint;
        waypoint.timeInSeconds = currentTime;
        waypoint.suitStates = currentStates; // Store all suit states

        // Add the waypoint to the spectrogram view
        spectrogramView->addWaypoint(waypoint);

        std::cout << "Waypoint added at " << currentTime << " seconds with states of all suits." << std::endl;
    });


    connect(templateAction, &QAction::triggered, this, [this]() {
        std::vector<SuitState> currentStates;

        // Collect the current state from each pictogram
        for (const auto& pictogram : pictograms) {
            if (pictogram) {
                currentStates.push_back(pictogram->getCurrentState());
            }
        }

        // Prompt user for preset name
        QString presetName = QInputDialog::getText(this, "Save Preset", "Enter preset name:");
        if (!presetName.isEmpty()) {
            // Save the preset to PresetManager and file
            presetManager.saveCurrentState(presetName.toStdString(), currentStates, "../resources/presets.json");
            presetManager.addPreset(Preset(presetName.toStdString(), currentStates));

            // Dynamically create the button and delete icon
            QWidget* buttonContainer = new QWidget();
            QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
            buttonLayout->setContentsMargins(0, 0, 0, 0);
            buttonLayout->setSpacing(5);

            // Create the delete button with a bin icon
            QPushButton* deleteButton = new QPushButton();
            deleteButton->setIcon(QIcon(":/icons/Bin.png"));
            deleteButton->setFixedSize(24, 24);
            deleteButton->setToolTip("Delete Preset");

            // Create the preset button
            QPushButton* presetButton = new QPushButton(presetName);
            presetButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

            // Add buttons to the layout
            buttonLayout->addWidget(deleteButton);
            buttonLayout->addWidget(presetButton);

            // Add the container to the main preset layout
            presetLayout->addWidget(buttonContainer);

            // Connect the preset button to apply the preset
            connect(presetButton, &QPushButton::clicked, this, [this, presetName]() {
                std::cout << "Preset clicked: " << presetName.toStdString() << std::endl;
                applyPreset(presetName.toStdString());
            });

            // Connect the delete button to delete the preset
            connect(deleteButton, &QPushButton::clicked, this, [this, presetName, buttonContainer]() {
                try {
                    // Remove preset and update the UI
                    deletePreset(presetName.toStdString());
                    presetLayout->removeWidget(buttonContainer);
                    buttonContainer->deleteLater();
                    std::cout << "Deleted preset: " << presetName.toStdString() << std::endl;
                } catch (const std::runtime_error& e) {
                    qWarning() << "Error deleting preset:" << e.what();
                }
            });

            qDebug() << "Added new preset dynamically: " << presetName;
        }
    });


    connect(stopAction, &QAction::triggered, this, &MainWindow::stopAudio);
    connect(fileAction, &QAction::triggered, this, &MainWindow::openFileExplorer);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::settings);
    connect(importAction, &QAction::triggered, this, &MainWindow::importFile);
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportFile);
    connect(distributeAction, &QAction::triggered, this, &MainWindow::distributeWaypoints);



    // Create a toolbar and add actions
    QToolBar* toolBar = new QToolBar(this);
    toolBar->setIconSize(QSize(64, 64)); // Set the icon size
    
    toolBar->addAction(settingsAction);
    toolBar->addAction(fileAction);
    toolBar->addAction(importAction);
    toolBar->addAction(exportAction);
    toolBar->addAction(playPauseAction);
    toolBar->addAction(stopAction);
    toolBar->addAction(templateAction);
    toolBar->addAction(addWaypointAction);
    toolBar->addAction(distributeAction);
    toolBar->addAction(lanTimingAction);

    // Add the toolbar to the main window
    addToolBar(Qt::TopToolBarArea, toolBar);




}


void MainWindow::setupPresets() {
    // Create a scrollable area
    scrollArea = new QScrollArea(leftWidget);
    scrollArea->setWidgetResizable(true);
    QWidget* scrollWidget = new QWidget();
    presetLayout = new QVBoxLayout(scrollWidget);

    scrollArea->setWidget(scrollWidget);
    QVBoxLayout* containerLayout = new QVBoxLayout(leftWidget);
    containerLayout->addWidget(scrollArea);
    leftWidget->setLayout(containerLayout);

    // Load presets from file
    presetManager.loadFromFile((QDir::currentPath() + "/config/presets.json").toStdString());
    createPresetButtons();
}


void MainWindow::createPresetButtons() {
    // Clear existing buttons
    QLayoutItem* child;
    while ((child = presetLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    // Align buttons to the top
    presetLayout->setAlignment(Qt::AlignTop);

    // Load and display each preset with a bin icon
    const auto& presets = presetManager.getPresets();
    for (const auto& preset : presets) {
        // Create a container for the button and delete icon
        QWidget* buttonContainer = new QWidget();
        QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(5);

        // Create the delete button with a bin icon
        QPushButton* deleteButton = new QPushButton();
        deleteButton->setIcon(QIcon(":/icons/Bin.png")); // Bin icon
        deleteButton->setFixedSize(24, 24); // Small button size
        deleteButton->setToolTip("Delete Preset");

        // Create the preset button
        QPushButton* presetButton = new QPushButton(QString::fromStdString(preset.getName()));
        presetButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        // Add buttons to the layout
        buttonLayout->addWidget(deleteButton);
        buttonLayout->addWidget(presetButton);

        // Add the container to the main preset layout
        presetLayout->addWidget(buttonContainer);

        // Connect the preset button to apply the preset
        connect(presetButton, &QPushButton::clicked, this, [this, preset]() {
            std::cout << "Preset clicked: " << preset.getName() << std::endl;
            applyPreset(preset.getName());
        });

        // Connect the delete button to delete the preset
        connect(deleteButton, &QPushButton::clicked, this, [this, preset, buttonContainer]() {
            try {
                // Remove preset from manager and save to file
                deletePreset(preset.getName());

                // Remove the widget from the layout and delete it
                presetLayout->removeWidget(buttonContainer);
                buttonContainer->deleteLater(); // Schedule safe deletion

                std::cout << "Deleted preset and updated UI dynamically: " << preset.getName() << std::endl;
            } catch (const std::runtime_error& e) {
                qWarning() << "Error deleting preset:" << e.what();
            }
        });

    }
}


void MainWindow::deletePreset(const std::string& presetName) {
    try {
        // Remove preset from the manager
        presetManager.removePreset(presetName);
        std::cout << "Removed preset: " << presetName << std::endl;

        // Save updated preset list to the file
        presetManager.saveToFile("../resources/presets.json");
    } catch (const std::runtime_error& e) {
        qWarning() << "Error deleting preset:" << e.what();
    }
}




void MainWindow::applyPreset(const std::string& presetName) {
    const Preset* preset = presetManager.getPreset(presetName);

    if (preset) {
        const auto& suitStates = preset->getSuitStates();

        qDebug() << "Applying preset:" << QString::fromStdString(presetName);
        qDebug() << "Number of pictograms:" << pictograms.size();
        qDebug() << "Number of suit states:" << suitStates.size();

        if (suitStates.size() != pictograms.size()) {
            qWarning() << "Mismatch between suit states and pictograms!";
            return;
        }

        for (size_t i = 0; i < pictograms.size(); ++i) {
            pictograms[i]->setAllColors(suitStates[i]);

            // Force the pictogram to repaint
            pictograms[i]->update(); // Ensure the item itself is updated
            if (pictograms[i]->scene()) {
                pictograms[i]->scene()->update(); // Ensure the scene is also updated
            }
        }
    } else {
        qWarning() << "Preset not found:" << QString::fromStdString(presetName);
    }
}



void MainWindow::loadMusicFile(const std::string& filePath) {
    try {
        AudioPreprocessor preprocessor;
        int fftSize = 1024;         // Example FFT size
        int maxFrequency = 48000;  // Max frequency

        // Load the audio file
        if (!preprocessor.loadFile(filePath, 48000, fftSize)) {
            throw std::runtime_error("Failed to load audio file.");
        }

        // Retrieve audio duration
        float audioDuration = preprocessor.getAudioDuration(); // Add this method in AudioPreprocessor if missing

        // Compute the spectrogram
        preprocessor.computeSpectrogram(maxFrequency);

        // Get the computed spectrogram
        const auto& spectrogram = preprocessor.getSpectrogram();

        // Transpose the spectrogram if needed
        size_t frequencyBins = spectrogram.size();
        size_t timeFrames = spectrogram[0].size();
        std::vector<std::vector<float>> transposedSpectrogram(timeFrames, std::vector<float>(frequencyBins, 0.0f));

        for (size_t i = 0; i < frequencyBins; ++i) {
            for (size_t j = 0; j < timeFrames; ++j) {
                transposedSpectrogram[j][i] = spectrogram[i][j];
            }
        }

        // Load the transposed spectrogram into the view
        spectrogramView->loadSpectrogram(transposedSpectrogram, 48000, maxFrequency, audioDuration);

        // Debug: Verify correct orientation
        std::cout << "Spectrogram loaded with "
                  << transposedSpectrogram.size() << " time frames and "
                  << transposedSpectrogram[0].size() << " frequency bins. "
                  << "Duration: " << audioDuration << " seconds." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error loading music file: " << e.what() << std::endl;
    }
}






void MainWindow::setupPictogramGrid() {
    std::vector<QGraphicsScene*> scenes(numSuits);
    std::vector<QGraphicsView*> views(numSuits);

    QGridLayout* gridLayout = new QGridLayout(rightWidget);
    pictograms.resize(numSuits);

    for (int i = 0; i < numSuits; ++i) {
        // Create and initialize the scene and view
        QGraphicsScene* scene = new QGraphicsScene(rightWidget);
        QGraphicsView* view = new QGraphicsView(scene, rightWidget);
        QColor secondaryColor = (i < suitSecondaryColors.size()) ? suitSecondaryColors[i] : QColor("red");

        scenes[i] = scene;
        views[i] = view;

        // Configure the view
        views[i]->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        views[i]->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        views[i]->setInteractive(true);
        views[i]->setDragMode(QGraphicsView::NoDrag);
        views[i]->setTransformationAnchor(QGraphicsView::NoAnchor);
        views[i]->setResizeAnchor(QGraphicsView::NoAnchor);
        views[i]->setFrameStyle(QFrame::NoFrame);

        // Create a new LedSuitPictogram and add it to the scene
        pictograms[i] = new LedSuitPictogram(":/icons/man.svg", scenes[i], secondaryColor);
        pictograms[i]->setId(i);

        // Add the view to the grid layout
        gridLayout->addWidget(views[i], i / 4, i % 4);

        // Add a text label to the scene
        QString suitName = (i < suitNames.size()) ? suitNames[i] : QString("Suit %1").arg(i + 1); // Fallback if name is missing
        QGraphicsTextItem* textItem = new QGraphicsTextItem(suitName);
        textItem->setDefaultTextColor(Qt::black); // Set text color
        QFont font = textItem->font();
        font.setPointSize(150); // Adjust font size as needed
        font.setBold(true); // Make the font bold
        textItem->setFont(font);

        // Rotate the text for vertical orientation
        QTransform transform;
        transform.rotate(-90); // Rotate counterclockwise
        textItem->setTransform(transform);

        // Position the text to the left and slightly down from the pictogram
        qreal textOffsetX = 100; // Horizontal offset (to the left)
        qreal textOffsetY = 150; // Vertical offset (downward)
        QRectF pictogramBounds = pictograms[i]->boundingRect();
        textItem->setPos(pictogramBounds.left() - textOffsetX, 
                         pictogramBounds.top() + pictogramBounds.height() / 2 + textOffsetY);
        scenes[i]->addItem(textItem);

        // Defer scaling and size adjustments
        QTimer::singleShot(0, this, [this, view, scene, pictogram = pictograms[i]]() {
            qreal widgetHeight = rightWidget->height();
            qreal boxHeight = widgetHeight * 0.5;
            qreal boxWidth = boxHeight * 0.8;

            view->setFixedSize(QSize(boxWidth, boxHeight));

            // Adjust scene rect to match the SVG item's bounding box
            QRectF svgBounds = pictogram->boundingRect();
            scene->setSceneRect(svgBounds);

            // Scale the SVG to fit within the view
            view->fitInView(svgBounds, Qt::KeepAspectRatio);

            // Center the SVG within the scene
            qreal offsetX = (scene->sceneRect().width() - svgBounds.width()) / 2.0;
            qreal offsetY = (scene->sceneRect().height() - svgBounds.height()) / 2.0;
            scene->setSceneRect(svgBounds.translated(offsetX, offsetY));
        });
    }

    // Apply the grid layout to the rightWidget
    rightWidget->setLayout(gridLayout);
}





void MainWindow::loadTestData() {
    // Load test spectrogram data
    std::vector<std::vector<float>> testData(513, std::vector<float>(10000, 0.0f));

    // Fill with dummy data (e.g., gradient)
    for (size_t i = 0; i < testData.size(); ++i) {
        for (size_t j = 0; j < testData[i].size(); ++j) {
            testData[i][j] = static_cast<float>(i) / 513.0f * std::sin(j / 100.0);
        }
    }

    float testDuration = 100.0f; // Example duration in seconds for the test data
    spectrogramView->loadSpectrogram(testData, 48000, 1000, testDuration); // Sample rate: 48 kHz, Max Frequency: 1 kHz
}


void MainWindow::playAudio() {
    if (!audioPlayer->isPlaying()) {
        std::cout << "Audio is not playing. Starting playback." << std::endl;
        audioPlayer->play();
        connect(updateTimer, &QTimer::timeout, spectrogramView, &SpectrogramView::updateCursor);
        updateTimer->start(5); // Sync cursor updates every 30 ms
    } else {
        std::cout << "Audio is already playing." << std::endl;
    }
}



void MainWindow::pauseAudio() {
    if (audioPlayer->isPlaying()) {
        audioPlayer->pause();
        updateTimer->stop(); // Stop cursor updates
    }
}


void MainWindow::stopAudio() {
    // Always stop the audio player
    std::cout << "Attempting to stop audio playback." << std::endl;
    audioPlayer->stop();

    // Stop the cursor update timer
    updateTimer->stop();

    // Reset the play/pause button to the "Play" state
    playPauseAction->setIcon(QIcon(":/icons/Play.png"));
    playPauseAction->setText("Play");

    // Reset the spectrogram cursor to the start
    spectrogramView->updateCursor();

    std::cout << "Audio playback stopped and reset." << std::endl;
}


void MainWindow::openFileExplorer() {
    std::cout << "opened File Explorer" << std::endl;

}


void MainWindow::exportFile() {
    qWarning() << "Export process started.";
    
    if (!spectrogramView) {
        qWarning() << "SpectrogramView is not initialized!";
        return;
    }

    // 1) Fetch the shared pointers
    const auto& sharedWaypoints = spectrogramView->getWaypoints();

    // 2) Unwrap them into a plain std::vector<Waypoint>
    std::vector<Waypoint> unwrapped;
    unwrapped.reserve(sharedWaypoints.size());
    for (const auto& ptr : sharedWaypoints) {
        if (ptr) {
            unwrapped.push_back(*ptr); // dereference the shared_ptr
        }
    }

    // 3) Now call your existing serializer that requires std::vector<Waypoint>
    QJsonArray waypointsArray = serializeWaypoints(unwrapped);

    // ... your existing file dialog & save code ...
    qWarning() << "Waypoints serialized. Creating JSON document.";
    QJsonDocument doc(waypointsArray);

    qWarning() << "Opening save file dialog.";
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Waypoints"),
        "",
        tr("JSON Files (*.json)"),
        nullptr,
        QFileDialog::DontUseNativeDialog);

    if (fileName.isEmpty()) {
        qWarning() << "No file selected for export.";
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << fileName;
        return;
    }

    file.write(doc.toJson());
    file.close();
    qWarning() << "Export process completed. File exported to:" << fileName;
}




void MainWindow::importFile() {
    qWarning() << "Import process started.";

    // Open file dialog
    qWarning() << "About to call QFileDialog again...";
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Waypoints File"), "", tr("JSON Files (*.json)"));
    qWarning() << "Returned from QFileDialog with:" << fileName;
    if (fileName.isEmpty()) {
        qWarning() << "No file selected for import.";
        return;
    }

    qWarning() << "File selected:" << fileName;

    // Read file
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << fileName;
        QMessageBox::critical(this, tr("Import Error"), tr("Could not open file for reading."));
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();
    qWarning() << "File read successfully.";

    // Parse JSON
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isArray()) {
        qWarning() << "Invalid JSON format: Expected an array.";
        QMessageBox::critical(this, tr("Import Error"), tr("Invalid JSON format. Expected an array of waypoints."));
        return;
    }

    QJsonArray waypointsArray = doc.array();
    std::vector<Waypoint> newWaypoints = deserializeWaypoints(waypointsArray);

        // 1) Prompt: Overwrite or Append?
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Import Waypoints",
            "Do you want to overwrite existing waypoints or append to them?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
            QMessageBox::Yes
        );
        // Typically: Yes = Overwrite, No = Append, Cancel = do nothing

        if (reply == QMessageBox::Cancel) {
            qWarning() << "User cancelled import.";
            return;
        }
        if (reply == QMessageBox::Yes) {
            // Overwrite
            spectrogramView->clearWaypoints();
        } 
        // If reply == No, we do "Append"

        // 2) Add new waypoints
        for (const auto& waypoint : newWaypoints) {
            spectrogramView->addWaypoint(waypoint);
        }

        qWarning() << "Import process completed. Waypoints loaded.";
    }

void MainWindow::settings() {
    QString configFilePath = QDir::currentPath() + "/config/suits.json";

    SettingsDialog dialog(configFilePath, this);
    dialog.exec();

    // Reload the config after editing (if needed for live updates)
    loadSuitConfig(configFilePath);
}




void MainWindow::applySuitState(LedSuitPictogram* ledSuit, const SuitState& state) {
    ledSuit->setAllColors(state);
}


void MainWindow::applyWaypointState(const std::vector<SuitState>& suitStates) {
    if (suitStates.size() != pictograms.size()) {
        qWarning() << "Mismatch between suit states and pictograms!";
        return;
    }

    for (size_t i = 0; i < pictograms.size(); ++i) {
        if (pictograms[i]) {
            pictograms[i]->setAllColors(suitStates[i]);

            // Force the pictogram to repaint
            pictograms[i]->update();
            if (pictograms[i]->scene()) {
                pictograms[i]->scene()->update();
            }
        }
    }
}


void MainWindow::distributeWaypoints() {
    try {
        // Compress the waypoints
        auto compressedData = waypointCompressor->compressWaypoints();

        // Ensure the number of connections matches the number of suits
        if (compressedData.size() != suitConnections.size()) {
            throw std::runtime_error("Mismatch between number of suits and IP addresses.");
        }

        // Distribute data to each suit
        for (size_t suitIndex = 0; suitIndex < compressedData.size(); ++suitIndex) {
            const auto& [ip, port] = suitConnections[suitIndex];

            TcpClient client(ip, port);
            if (!client.connectToServer()) {
                throw std::runtime_error("Failed to connect to suit at " + ip.toStdString());
            }

            if (!client.sendData(compressedData[suitIndex])) {
                throw std::runtime_error("Failed to send data to suit at " + ip.toStdString());
            }

            client.disconnectFromServer();
        }

        QMessageBox::information(this, "Distribute Waypoints", "Waypoints have been distributed successfully.");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to distribute waypoints: %1").arg(e.what()));
    }
}





void MainWindow::synchronizeAllDevices() {
    try {
        // Create a UDP socket
        QUdpSocket udpSocket;

        // Prepare the "go" signal as a simple string or byte
        QByteArray goSignal = "GO";

        // Broadcast the signal to all devices
        const QHostAddress broadcastAddress = QHostAddress("192.168.105.255"); // Replace with your network's broadcast address
        const quint16 udpPort = 12346; // The port all ESP32 devices are listening on

        if (udpSocket.writeDatagram(goSignal, broadcastAddress, udpPort) == -1) {
            throw std::runtime_error("Failed to broadcast start signal.");
        }

        qWarning() << "Broadcasted 'GO' signal to" << broadcastAddress.toString() << ":" << udpPort;
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to synchronize devices: %1").arg(e.what()));
    }
}



