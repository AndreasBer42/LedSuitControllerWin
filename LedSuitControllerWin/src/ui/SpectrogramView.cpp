#include "include/ui/SpectrogramView.h"
#include "include/core/AudioPlayer.h"
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QImage>
#include <QPainter>
#include <cmath>
#include <iostream>
#include <QScrollBar>
#include <chrono>
#include <QDebug>
#include <QStyleOptionGraphicsItem>
#include <QStyle>
#include <QGraphicsSceneMouseEvent>

SpectrogramView::SpectrogramView(QWidget* parent)
    : QGraphicsView(parent), scene(new QGraphicsScene(this)), zoomLevel(1.0f), currentOffset(0), blockWaypointUpdates(false) {
    setScene(scene);

    // Create a layer for the spectrogram
    spectrogramItem = new QGraphicsPixmapItem();
    scene->addItem(spectrogramItem);

    // Create a layer for the cursor
    cursorItem = new QGraphicsPixmapItem();
    scene->addItem(cursorItem);

    // Create a layer for waypoints
    waypointLayer = new QGraphicsItemGroup();
    scene->addItem(waypointLayer);

    // Disable the vertical scrollbar
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setInteractive(true);

}







void SpectrogramView::loadSpectrogram(const std::vector<std::vector<float>>& data, int sampleRate, int maxFrequency, float audioDuration) {
    spectrogramData = data;
    this->sampleRate = sampleRate;
    this->maxFrequency = maxFrequency;
    this->duration = audioDuration; // Set the duration

    float scalingFactor = 1;
    // Calculate target resolution using the scaling factor
    int originalTimeFrames = static_cast<int>(data[0].size());
    int originalFrequencyBins = static_cast<int>(data.size());

    int targetTimeFrames = static_cast<int>(originalTimeFrames * scalingFactor);
    int targetFrequencyBins = static_cast<int>(originalFrequencyBins * scalingFactor);

    std::cout << "Original Time Frames: " << originalTimeFrames 
              << ", Original Frequency Bins: " << originalFrequencyBins << "\n";
    std::cout << "Target Time Frames: " << targetTimeFrames 
              << ", Target Frequency Bins: " << targetFrequencyBins << "\n";

    // Downsample the spectrogram data
    spectrogramData = downsampleSpectrogram(data, targetTimeFrames, targetFrequencyBins);


    std::cout << "Spectrogram time frames: " << spectrogramData[0].size()
          << ", Audio duration: " << duration << " seconds"
          << ", Frames per second: " << spectrogramData[0].size() / duration << std::endl;


    // Update the view
    updateView();
    updateWaypointPositions();
}



void SpectrogramView::setZoomLevel(float zoom) {
    zoomLevel = std::max(0.1f, zoom);
    qWarning() << "Zoom Level is : " << zoomLevel ;
    updateView();
    updateWaypointPositions();
}



void SpectrogramView::scrollBy(int deltaX) {
    blockWaypointUpdates = true; // Block waypoint updates during manual scrolling

    int previousOffset = currentOffset;
    currentOffset = std::clamp(currentOffset + deltaX, 0, getTimeFrames() - static_cast<int>(width() / zoomLevel));
    std::cout << "ScrollBy called. Delta: " << deltaX 
              << ", Previous offset: " << previousOffset
              << ", New offset: " << currentOffset << std::endl;
    updateView();
    updateWaypointPositions();

    blockWaypointUpdates = false; // Re-enable waypoint updates
}





void SpectrogramView::wheelEvent(QWheelEvent* event) {
    blockWaypointUpdates = true; // Block waypoint updates during manual scrolling/zooming

    if (event->modifiers() & Qt::ControlModifier) {
        int rawDelta = event->angleDelta().y();
        int scrollAmount = static_cast<int>(-rawDelta * (1 / zoomLevel) * 1); // Adjust scaling factor as needed

        scrollBy(scrollAmount);
    } else {
        // Zoom while keeping the mouse pointer static
        QPoint mousePosition = event->position().toPoint();
        int mouseColumn = currentOffset + static_cast<int>(mousePosition.x() / zoomLevel);

        // Adjust zoom level
        float previousZoomLevel = zoomLevel;
        float fullFitZoomLevel = width() / static_cast<float>(getTimeFrames()); 
        zoomLevel = std::max(fullFitZoomLevel, zoomLevel + event->angleDelta().y() / 3600.0f);

        // Recalculate offset to keep the zoom centered on the mouse pointer
        int visibleColumnsBefore = static_cast<int>(width() / previousZoomLevel);
        int visibleColumnsAfter = static_cast<int>(width() / zoomLevel);

        currentOffset = mouseColumn - static_cast<int>((mouseColumn - currentOffset) * (visibleColumnsAfter / static_cast<float>(visibleColumnsBefore)));
        currentOffset = std::clamp(currentOffset, 0, getTimeFrames() - visibleColumnsAfter);

        updateView();
        updateWaypointPositions();
    }

    blockWaypointUpdates = false; // Re-enable waypoint updates after manual scrolling/zooming
}




void SpectrogramView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        // Check if any waypoint is selected
        for (size_t i = 0; i < waypointItems.size(); ++i) {
            auto* lineItem = waypointItems[i];
            if (lineItem && lineItem->isSelected()) {
                // Remove the item from the scene
                scene->removeItem(lineItem);
                delete lineItem;

                // Remove the corresponding waypoint from the vectors
                waypointItems.erase(waypointItems.begin() + i);
                waypoints.erase(waypoints.begin() + i);

                // Update the positions of remaining waypoints
                updateWaypointPositions();
                return; // Exit after deleting the first selected waypoint
            }
        }
    }

    // Pass the event to the base class for default behavior
    QGraphicsView::keyPressEvent(event);
}



void SpectrogramView::updateView() {
    if (spectrogramData.empty()) {
        std::cerr << "Spectrogram data is empty!" << std::endl;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Range Calculation
    int visibleColumns = static_cast<int>(width() / zoomLevel);
    visibleColumns = std::clamp(visibleColumns, 1, static_cast<int>(spectrogramData[0].size()));
    int maxOffset = static_cast<int>(spectrogramData[0].size()) - visibleColumns;
    currentOffset = std::clamp(currentOffset, 0, maxOffset);
    int startColumn = currentOffset;
    int endColumn = std::min(startColumn + visibleColumns, static_cast<int>(spectrogramData[0].size()));

    // Rendering Preparation
    QImage spectrogramImage(width(), height(), QImage::Format_RGB32);
    QPainter painter(&spectrogramImage);

    // Render Spectrogram
    for (int y = 0; y < height() - 20; ++y) {
        int binIndex = static_cast<int>(((height() - y - 1) / static_cast<float>(height() - 20)) * spectrogramData.size());
        binIndex = std::min(binIndex, static_cast<int>(spectrogramData.size()) - 1);

        for (int x = 0; x < width(); ++x) {
            int sourceColumn = startColumn + static_cast<int>((x / static_cast<float>(width())) * visibleColumns);
            float normalizedAmplitude = spectrogramData[binIndex][sourceColumn];
            int intensity = static_cast<int>(normalizedAmplitude * 255);
            int hue = std::clamp(240 - intensity, 0, 240);
            int saturation = 255;
            int value = std::clamp(intensity, 0, 255);

            QColor color = QColor::fromHsv(hue, saturation, value);
            spectrogramImage.setPixel(x, y, color.rgb());
        }
    }



    // Fill Time Axis Background
    painter.fillRect(0, height() - 20, width(), 20, Qt::black);

    // Render Time Axis
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 8));
    float secondsPerColumn = duration / static_cast<float>(spectrogramData[0].size());
    for (int x = 0; x < width(); x += 100) {
        int sourceColumn = startColumn + static_cast<int>((x / static_cast<float>(width())) * visibleColumns);
        float timeInSeconds = sourceColumn * secondsPerColumn;
        QString timeLabel = QString::number(timeInSeconds, 'f', 1) + "s";
        painter.drawText(x, height() - 5, timeLabel);
    }

    painter.end();

    // Update Spectrogram Scene Layer
    if (!spectrogramItem) {
        spectrogramItem = new QGraphicsPixmapItem();
        scene->addItem(spectrogramItem);
    }
    spectrogramItem->setPixmap(QPixmap::fromImage(spectrogramImage));
    setSceneRect(0, 0, spectrogramImage.width(), spectrogramImage.height());

    // Update Cursor Layer
    updateCursorLayer();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Total Update: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
}




void SpectrogramView::updateCursorLayer() {
    if (!cursorItem) {
        cursorItem = new QGraphicsPixmapItem();
        scene->addItem(cursorItem);
    }

    // Clear the cursor image
    QImage cursorImage(width(), height(), QImage::Format_ARGB32);
    cursorImage.fill(Qt::transparent);

    // Render the cursor
    QPainter painter(&cursorImage);
    renderCursor(cursorImage, painter);
    painter.end();

    // Update the cursor layer
    cursorItem->setPixmap(QPixmap::fromImage(cursorImage));
}




void SpectrogramView::renderCursor(QImage &image, QPainter &painter) {
    if (cursorPosition < 0) return; // No cursor to render if position is invalid

    int visibleColumns = static_cast<int>(width() / zoomLevel);
    int startColumn = currentOffset;
    int endColumn = std::min(startColumn + visibleColumns, static_cast<int>(spectrogramData[0].size()));

    int cursorColumn = static_cast<int>(cursorPosition);
    if (cursorColumn >= startColumn && cursorColumn < endColumn) {
        // Calculate cursor X position on screen
        float cursorX = ((cursorColumn - startColumn) / static_cast<float>(visibleColumns)) * width();

        // Draw the cursor as a red line
        painter.setPen(QPen(Qt::red, 2));
        painter.drawLine(QPointF(cursorX, 0), QPointF(cursorX, height() - 20));
    }
}





void SpectrogramView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    updateView();
    updateWaypointPositions();
}

int SpectrogramView::getFrequencyBins() const {
    return static_cast<int>(spectrogramData.size());
}

int SpectrogramView::getTimeFrames() const {
    return spectrogramData.empty() ? 0 : static_cast<int>(spectrogramData[0].size());
}

const std::vector<std::shared_ptr<Waypoint>>& SpectrogramView::getWaypoints() const {
    return waypoints;
}


std::pair<int, int> SpectrogramView::getViewRange() const {
    int visibleColumns = static_cast<int>(width() / zoomLevel);
    int startColumn = std::max(0, currentOffset);
    int endColumn = std::min(startColumn + visibleColumns, getTimeFrames());
    return {startColumn, endColumn};
}


std::vector<std::vector<float>> SpectrogramView::downsampleSpectrogram(
    const std::vector<std::vector<float>>& data,
    int targetTimeFrames,
    int targetFrequencyBins) {
    
    int originalTimeFrames = static_cast<int>(data[0].size());
    int originalFrequencyBins = static_cast<int>(data.size());

    // Compute downsampling factors
    float timeStep = static_cast<float>(originalTimeFrames) / targetTimeFrames;
    float frequencyStep = static_cast<float>(originalFrequencyBins) / targetFrequencyBins;

    // Create a new downsampled spectrogram
    std::vector<std::vector<float>> downsampled(targetFrequencyBins, std::vector<float>(targetTimeFrames, 0.0f));

    for (int i = 0; i < targetFrequencyBins; ++i) {
        int originalFreqIndexStart = static_cast<int>(i * frequencyStep);
        int originalFreqIndexEnd = std::min(static_cast<int>((i + 1) * frequencyStep), originalFrequencyBins);

        for (int j = 0; j < targetTimeFrames; ++j) {
            int originalTimeIndexStart = static_cast<int>(j * timeStep);
            int originalTimeIndexEnd = std::min(static_cast<int>((j + 1) * timeStep), originalTimeFrames);

            // Average over the original data range
            float sum = 0.0f;
            int count = 0;

            for (int fi = originalFreqIndexStart; fi < originalFreqIndexEnd; ++fi) {
                for (int ti = originalTimeIndexStart; ti < originalTimeIndexEnd; ++ti) {
                    sum += data[fi][ti];
                    ++count;
                }
            }

            float average = (count > 0) ? (sum / count) : 0.0f;

            // Apply logarithmic scaling
            downsampled[i][j] = (std::log10(std::max(average, 0.01f)) - std::log10(0.01f)) /
                                (std::log10(10.0f) - std::log10(0.01f));
        }
    }

    return downsampled;
}

void SpectrogramView::connectAudioPlayer(AudioPlayer* player) {
    if (player) {
        audioPlayer = player;
        connect(audioPlayer, &AudioPlayer::playbackPositionChanged, this, &SpectrogramView::updateCursorFromAudio);
        std::cout << "AudioPlayer connected successfully." << std::endl;
    } else {
        std::cerr << "Failed to connect AudioPlayer: nullptr passed." << std::endl;
    }
}





void SpectrogramView::updateCursor() {
    if (!scene || !audioPlayer || duration <= 0.0f) return;

    // Get the current playback time from the audio player
    float currentTime = audioPlayer->getCurrentTime();

    // Calculate the cursor position based on playback time
    cursorPosition = (currentTime / duration) * spectrogramData[0].size();

    // Ensure the cursor stays within bounds
    cursorPosition = std::clamp(cursorPosition, 0.0f, static_cast<float>(spectrogramData[0].size() - 1));

    // Check if the cursor is out of the visible range
    int visibleColumns = static_cast<int>(width() / zoomLevel);
    int startColumn = currentOffset;
    int endColumn = std::min(startColumn + visibleColumns, static_cast<int>(spectrogramData[0].size()));

    if (cursorPosition >= endColumn) {
        std::cout << "Cursor out of view on the right. Scrolling forward." << std::endl;
        scrollBy(endColumn - startColumn); // Scroll forward
        updateView(); // Update the full view
    } else if (cursorPosition < startColumn) {
        std::cout << "Cursor out of view on the left. Scrolling backward." << std::endl;
        scrollBy(1.6 * (startColumn - endColumn)); // Scroll backward
        updateView(); // Update the full view
    } else {
        // Cursor is within the visible range; update only the cursor layer
        updateCursorLayer();
    }

    // Find the last waypoint before or at the cursor position
    std::shared_ptr<Waypoint> lastWaypoint = nullptr;
    for (const auto& waypoint : waypoints) {
        if (waypoint->timeInSeconds <= currentTime) {
            lastWaypoint = waypoint;
        } else {
            break;
        }
    }

    // Emit the signal only if the waypoint has changed
    if (lastWaypoint && lastWaypoint != lastEmittedWaypoint) {
        lastEmittedWaypoint = lastWaypoint; // Update the last emitted waypoint
        qWarning() << "last waypoint time = " << lastWaypoint->timeInSeconds;
                                            
        emit updatePictograms(lastWaypoint->suitStates);
    }
}



void SpectrogramView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // Get the item under the mouse cursor
        qWarning() << "clicked";
        QGraphicsItem* clickedItem = itemAt(event->pos());
        if (clickedItem) {
            // Check if the clicked item is a SelectableWaypoint
            qWarning() << "clickeditem" << clickedItem;
            auto* waypoint = dynamic_cast<SelectableWaypoint*>(clickedItem);
            if (waypoint) {
                // Allow QGraphicsScene to handle the selection of the waypoint
                QGraphicsView::mousePressEvent(event); // Pass the event to the base class
                std::cout << "Waypoint selected!" << std::endl;
                return;
            }
        }

        // If no waypoint is clicked, handle cursor logic
        int clickX = event->pos().x();
        int visibleColumns = static_cast<int>(width() / zoomLevel);
        int startColumn = currentOffset;

        int clickedColumn = startColumn + static_cast<int>((clickX / static_cast<float>(width())) * visibleColumns);
        clickedColumn = std::clamp(clickedColumn, 0, getTimeFrames() - 1);

        float newPlaybackTime = (clickedColumn / static_cast<float>(spectrogramData[0].size())) * duration;

        if (audioPlayer) {
            audioPlayer->seek(newPlaybackTime);
        }

        std::cout << "Mouse clicked at column: " << clickedColumn
                  << ", New playback time: " << newPlaybackTime << "s." << std::endl;

        cursorPosition = static_cast<float>(clickedColumn);
        updateCursor();
        updateCursorLayer();
    }

    // Pass the event to the base class for default behavior
    QGraphicsView::mousePressEvent(event);
}



void SpectrogramView::updateCursorFromAudio(double currentTime) {
    cursorPosition = (currentTime / duration) * spectrogramData[0].size();
    updateCursorLayer();
}



void SpectrogramView::addWaypoint(const Waypoint& waypoint)
{
    if (duration <= 0.0f) {
        return;
    }

    if (audioPlayer && audioPlayer->isPlaying()) {
        qWarning() << "Playback is active. Skipping waypoint addition.";
        return;
    }

    // Create a shared_ptr for the new waypoint
    auto waypointPtr = std::make_shared<Waypoint>(waypoint);
    waypoints.push_back(waypointPtr);

    // Sort waypoints by time
    std::sort(waypoints.begin(), waypoints.end(), [](const std::shared_ptr<Waypoint>& a, const std::shared_ptr<Waypoint>& b) {
        return a->timeInSeconds < b->timeInSeconds;
    });

    // Rebuild waypointItems to match sorted waypoints
    std::vector<QGraphicsLineItem*> sortedItems;
    for (const auto& sortedWaypoint : waypoints) {
        auto it = std::find_if(waypointItems.begin(), waypointItems.end(), [&](QGraphicsLineItem* item) {
            auto* selectableWaypoint = dynamic_cast<SelectableWaypoint*>(item);
            return selectableWaypoint && selectableWaypoint->getWaypoint() == sortedWaypoint;
        });

        if (it != waypointItems.end()) {
            sortedItems.push_back(*it);
        } else {
            // If no graphical item exists for this waypoint, create one
            auto* waypointLine = new SelectableWaypoint(sortedWaypoint);
            waypointLine->setLine(0, 0, 0, height() - 20);
            scene->addItem(waypointLine);
            sortedItems.push_back(waypointLine);
        }
    }

    // Replace old waypointItems with the sorted ones
    waypointItems = std::move(sortedItems);

    // Update positions for all waypoint items
    updateWaypointPositions();

    // Emit the waypointAdded signal
    emit waypointAdded(*waypointPtr);
}




void SpectrogramView::clearWaypoints()
{
    // Remove old waypoint items from the scene
    for (auto* item : waypointItems) {
        if (item && scene) {
            scene->removeItem(item);
            delete item; // or let Qt handle if items have no parent
        }
    }
    waypointItems.clear();

    // Clear the underlying data
    waypoints.clear();

    // Possibly reset cursor if desired
    cursorPosition = -1.0f;

    // Force an update
    updateView();
    updateCursorLayer();
    updateWaypointPositions();
}




void SpectrogramView::updateWaypointPositions()
{


    if (waypoints.empty() || duration <= 0.0f) {
        return;
    }

    float visibleColumns = width() / zoomLevel;
    float totalColumns = spectrogramData[0].size();

    for (int i = 0; i < static_cast<int>(waypoints.size()); ++i) {
        const auto& waypoint = waypoints[i];

        // Calculate the column position based on the waypoint's time
        float timeRatio = waypoint->timeInSeconds / duration;
        float waypointColumn = timeRatio * totalColumns;

        // Convert column to screen X for the current offset and zoom
        float waypointX = ((waypointColumn - currentOffset) / visibleColumns) * width();

        if (auto* lineItem = waypointItems[i]) {
            // Update the item's graphical position
            lineItem->setLine(0, 0, 0, height() - 20);
            lineItem->setPos(waypointX, 0);

            if (waypointX >= 0 && waypointX < width()) {
                lineItem->show();
            } else {
                lineItem->hide();
            }

            // Debug log for verification
            qWarning() << "Waypoint " << i << " time: " << waypoint->timeInSeconds
                       << ", Position: " << waypointX;
        }
    }
}






float SpectrogramView::mapXToTime(float x) const
{
    qWarning() << "=== mapXToTime Debug ===";
    qWarning() << "x =" << x;
    qWarning() << "width() =" << width();
    qWarning() << "zoomLevel =" << zoomLevel;
    qWarning() << "currentOffset =" << currentOffset;
    // Check if spectrogramData is empty before accessing spectrogramData[0]
    size_t timeFrames = (!spectrogramData.empty() ? spectrogramData[0].size() : 1);
    qWarning() << "spectrogramData[0].size() =" << timeFrames;
    qWarning() << "duration =" << duration;

    float visibleColumns = width() / zoomLevel;
    qWarning() << "visibleColumns =" << visibleColumns;

    float timePerColumn = duration / static_cast<float>(timeFrames);
    float startTime = currentOffset * timePerColumn;
    float endTime   = (currentOffset + visibleColumns) * timePerColumn;
    float timePerPixel = (endTime - startTime) / width();

    qWarning() << "startTime =" << startTime;
    qWarning() << "endTime =" << endTime;
    qWarning() << "timePerPixel =" << timePerPixel;
    qWarning() << "========================";

    // Now compute the final time from x
    return startTime + x * timePerPixel;
}













SelectableWaypoint::SelectableWaypoint(std::shared_ptr<Waypoint> waypoint,
                                       QGraphicsItem* parent)
    : QGraphicsLineItem(parent)
    , waypointPtr(std::move(waypoint))
{
    // Give it some default geometry, e.g. vertical line from (0, 0) to (0, 100)
    setLine(0, 0, 0, 100);

    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);

    updateAppearance();
}

QPainterPath SelectableWaypoint::shape() const
{
    QPainterPath path;
    QPen pen = this->pen();
    qreal extraWidth = pen.widthF() + 20.0; // Increase hitbox
    QLineF line = this->line();

    QPainterPathStroker stroker;
    stroker.setWidth(extraWidth);

    path.moveTo(line.p1());
    path.lineTo(line.p2());
    return stroker.createStroke(path);
}


void SelectableWaypoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    updateAppearance();

    // Suppress the default selection outline
    if (option->state & QStyle::State_Selected) {
        QStyleOptionGraphicsItem customOption(*option);
        customOption.state &= ~QStyle::State_Selected; 
        QGraphicsLineItem::paint(painter, &customOption, widget);
    } else {
        QGraphicsLineItem::paint(painter, option, widget);
    }
}

void SelectableWaypoint::updateAppearance()
{
    //qWarning() << "Updating appearance. Selected:" << isSelected();
    if (isSelected()) {
        setPen(QPen(Qt::blue, 2));
    } else {
        setPen(QPen(Qt::yellow, 2));
    }
    update();
    if (scene()) {
        scene()->update(this->boundingRect());
    } else {
        qWarning() << "scene() is nullptr. Cannot update bounding rect.";
    }
}


QVariant SelectableWaypoint::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        QPointF newPos = value.toPointF();

        // Lock the Y coordinate so it remains horizontal only
        newPos.setY(0);

        // Optionally clamp x >= 0
        if (newPos.x() < 0) {
            newPos.setX(0);
        }
        return QGraphicsLineItem::itemChange(change, newPos);
    }
    else if (change == QGraphicsItem::ItemPositionHasChanged) {
        QPointF finalPos = this->pos(); 
        updateWaypointInList(finalPos);
        return QGraphicsLineItem::itemChange(change, value);
    }
    return QGraphicsLineItem::itemChange(change, value);
}



void SelectableWaypoint::updateWaypointInList(const QPointF& newPos) {
    qWarning() << "entered updateWaypointInList";

    // Ensure the SpectrogramView is found
    if (auto* spectrogramView = dynamic_cast<SpectrogramView*>(scene()->views().first())) {
        // Skip updates if modifications are blocked
        if (spectrogramView->blockWaypointUpdates) {
            qWarning() << "Waypoint updates are blocked. Skipping update.";
            return;
        }

        // Use the public getter for audioPlayer
        if (spectrogramView->getAudioPlayer() && spectrogramView->getAudioPlayer()->isPlaying()) {
            qWarning() << "Playback is active. Skipping waypoint update.";
            return;
        }

        float newTime = spectrogramView->mapXToTime(newPos.x());

        // Update only this waypoint's timeInSeconds
        if (waypointPtr) {
            waypointPtr->timeInSeconds = newTime;
            qWarning() << "Updated waypoint time to:" << newTime;
        } else {
            qWarning() << "waypointPtr is null!";
        }
    }
}

