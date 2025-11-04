#ifndef LEDSUITPICTOGRAM_H
#define LEDSUITPICTOGRAM_H

#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QGraphicsScene>
#include <QColor>
#include <QObject>
#include <QMap>
#include <QGraphicsSceneMouseEvent>
#include "include/core/SuitState.h"

// LedSuitPictogram class inheriting both QObject and QGraphicsSvgItem
class LedSuitPictogram : public QGraphicsSvgItem {
    Q_OBJECT

public:
    // Constructor
    explicit LedSuitPictogram(const QString& svgFilePath, QGraphicsScene* parentScene, 
                              const QColor& secondaryColor, QObject* parent = nullptr);

    // Set color for a specific part by ID
    void setPartColor(const QString& partId, const QColor& color);
    QRectF boundingRect() const override { return QGraphicsSvgItem::boundingRect(); }
    void setTransform(const QTransform& transform) { QGraphicsSvgItem::setTransform(transform); }
    void setAllColors(const SuitState& state);

    // Reset all parts to default color (e.g., black)
    void resetColors();

    void setId(int newId) { id = newId; }
    int getId() const { return id; }
    SuitState getCurrentState() const; // Method to retrieve the current state

protected:
    // Override mousePressEvent to handle clicks
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QSvgRenderer* renderer;                  // Renders the SVG
    QGraphicsScene* parentScene;             // Scene for rendering
    QMap<QString, QColor> partColors;        // Map to store current colors of parts
    QString svgPath;
    QString originalSvgContent;
    QMap<QString, QColor> currentColors;     // Tracks colors for each part
    QMap<QString, QRectF> partBounds;        // Map of part ID to bounding rectangles
    QColor secondaryColor;                   // Secondary color (red or green)
    void applyCurrentColors();
    void loadPartBounds(); // Method to precompute part bounds
    int id;                // Unique identifier for the pictogram
};

#endif // LEDSUITPICTOGRAM_H



