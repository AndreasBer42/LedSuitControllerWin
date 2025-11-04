#ifndef SPECTROGRAMVIEW_H
#define SPECTROGRAMVIEW_H

#include "include/core/SuitState.h"
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <vector>
#include <utility> // For std::pair
#include <QGraphicsSceneMouseEvent>
#include <memory>
                   

class AudioPlayer;
class QGraphicsItemGroup;

class SpectrogramView : public QGraphicsView {
    Q_OBJECT

public:
    explicit SpectrogramView(QWidget* parent = nullptr);
    AudioPlayer* getAudioPlayer() const { return audioPlayer; }

    void connectAudioPlayer(AudioPlayer* player); // Connects the spectrogram view to the audio player
    void updateCursor(); // Updates the cursor position on the spectrogram
    
    void loadSpectrogram(const std::vector<std::vector<float>>& data, int sampleRate, int maxFrequency, float audioDuration); // Loads the spectrogram data

    void setZoomLevel(float zoom); // Sets the zoom level of the spectrogram
    void scrollBy(int deltaX); // Scrolls the spectrogram horizontally

    // Utility functions for testing or querying state
    int getFrequencyBins() const; // Returns the number of frequency bins
    int getTimeFrames() const; // Returns the number of time frames
    const std::vector<std::shared_ptr<Waypoint>>& getWaypoints() const;                                
    std::pair<int, int> getViewRange() const; // Returns the visible range of the spectrogram
                                              
    void addWaypoint(const Waypoint& waypoint);
    void clearWaypoints();
    void updateWaypointPositions(); // Updates waypoint positions during view changes
                                    
    float mapXToTime(float x) const;
    bool blockWaypointUpdates; 

                                    


signals:
    void waypointAdded(const Waypoint& waypoint);
    void updatePictograms(const std::vector<SuitState>& suitStates);

public slots:
    void updateCursorFromAudio(double currentTime);


protected:
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override; // Handles mouse wheel events for zooming
    void resizeEvent(QResizeEvent* event) override; // Handles resize events for the view
    void keyPressEvent(QKeyEvent* event) override;                                                    
    void renderCursor(QImage &image, QPainter &painter);      
    void updateCursorLayer();


private:
    void updateView();
     
    std::vector<std::vector<float>> downsampleSpectrogram(const std::vector<std::vector<float>>& data, int targetTimeFrames, int targetFrequencyBins); // Downsamples the spectrogram for efficient rendering
    std::vector<std::shared_ptr<Waypoint>> waypoints;
    std::shared_ptr<Waypoint> lastEmittedWaypoint = nullptr;
    std::vector<QGraphicsLineItem*> waypointItems;  // Graphics items representing waypoints

    QGraphicsScene* scene; // Graphics scene for rendering
    QGraphicsPixmapItem *spectrogramItem = nullptr; // Layer for spectrogram rendering
    QGraphicsPixmapItem *cursorItem = nullptr;
    QGraphicsItemGroup* waypointLayer = nullptr; // Dedicated layer for waypoints
                                                 //
    AudioPlayer* audioPlayer; // Pointer to the connected audio player
    std::vector<std::vector<float>> spectrogramData; // Spectrogram data
    int sampleRate; // Sample rate of the audio
    int maxFrequency; // Maximum frequency of the spectrogramData

    float zoomLevel; // Current zoom level
    float cursorPosition; // Current cursor position in seconds
    int currentOffset; // Current horizontal offset for rendering
    float duration; // Total duration of the audio in seconds
    bool autoScroll; // Whether auto-scrolling is enable
};


class SelectableWaypoint : public QGraphicsLineItem {
public:
    // Instead of taking (Waypoint&), accept a shared_ptr<Waypoint>.
    SelectableWaypoint(std::shared_ptr<Waypoint> waypointPtr,
                       QGraphicsItem* parent = nullptr);

    // Getter for the associated waypoint
    std::shared_ptr<Waypoint> getWaypoint() const { return waypointPtr; }

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QPainterPath shape() const override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    void updateAppearance();
    void updateWaypointInList(const QPointF& newPos);

    std::shared_ptr<Waypoint> waypointPtr;
};
#endif // SPECTROGRAMVIEW_H


