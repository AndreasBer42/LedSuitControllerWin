#include "include/ui/LedSuitPictogram.h"
#include <QDebug>
#include <QPainter>
#include <QIODevice>
#include <QByteArray>
#include <QRegularExpression>
#include <QFile>
#include <iostream>



LedSuitPictogram::LedSuitPictogram(const QString& svgFilePath, QGraphicsScene* parentScene, const QColor& secondaryColor, QObject* parent)
    : QGraphicsSvgItem(), svgPath(svgFilePath), secondaryColor(secondaryColor) { // Store svgPath and initialize secondaryColor
    // Load the SVG content into memory
    QFile svgFile(svgFilePath);
    if (!svgFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open SVG file:" << svgFilePath;
        return;
    }

    originalSvgContent = svgFile.readAll(); // Store the original SVG content in memory
    svgFile.close();

    renderer = new QSvgRenderer(originalSvgContent.toUtf8(), this);

    if (!renderer->isValid()) {
        qWarning() << "Failed to load SVG file:" << svgFilePath;
        return;
    }

    // Set the renderer for this QGraphicsSvgItem
    setSharedRenderer(renderer);

    // Set interaction properties
    setAcceptHoverEvents(true);

    // Add this item directly to the scene
    parentScene->addItem(this);

    loadPartBounds();
}



void LedSuitPictogram::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QPointF clickPos = event->pos(); // Mouse position relative to the item

    for (auto it = partBounds.begin(); it != partBounds.end(); ++it) {
        if (it.value().contains(clickPos)) {
            QString partId = it.key();

            // Toggle color logic
            QColor currentColor = currentColors.value(partId, QColor("black"));
            QColor newColor;

            if (partId.contains("Primary", Qt::CaseInsensitive)) {
                // Toggle between black and blue for primary parts
                newColor = (currentColor == QColor("black")) ? QColor("blue") : QColor("black");
            } else if (partId.contains("Secondary", Qt::CaseInsensitive)) {
                // Toggle between black and the dynamically assigned secondary color
                newColor = (currentColor == QColor("black")) ? secondaryColor : QColor("black");
            } else if (partId.contains("Head", Qt::CaseInsensitive)) {
                // Toggle between black and red for the head
                newColor = (currentColor == QColor("black")) ? QColor("red") : QColor("black");
            } else {
                newColor = currentColor; // For parts without specific rules
            }

            setPartColor(partId, newColor); // Apply the new color

            // Force the item to repaint
            update();

            qWarning() << "Toggled color for part:" << partId << "to" << newColor.name();
            return; // Stop checking after the first match
        }
    }

    qWarning() << "Clicked on unknown area!";
    QGraphicsSvgItem::mousePressEvent(event);
}






void LedSuitPictogram::setPartColor(const QString& partId, const QColor& color) {
    if (!renderer->elementExists(partId)) {
        qWarning() << "Part ID" << partId << "does not exist in the SVG.";
        return;
    }

    // Update the color map
    currentColors[partId] = color;

    // Reload the SVG with the updated colors
    applyCurrentColors();
}



void LedSuitPictogram::applyCurrentColors() {
    QString svgContent = originalSvgContent; // Use the original SVG content from memory

    // Update the `fill` attribute for each part based on `currentColors`
    for (auto it = currentColors.begin(); it != currentColors.end(); ++it) {
        QString partId = it.key();
        QColor color = it.value();

        QString colorHex = color.name(QColor::HexRgb);
        QString targetElement = QString("id=\"%1\"").arg(partId);
        QRegularExpression fillRegex(QString("(<path[^>]*%1[^>]*style=\"[^\"]*fill:)(#[0-9A-Fa-f]{6})").arg(targetElement));

        QRegularExpressionMatch match = fillRegex.match(svgContent);
        if (match.hasMatch()) {
            svgContent.replace(fillRegex, QString("\\1%2").arg(colorHex));
        } else {
            // qWarning() << "Fill attribute for part ID" << partId << "not found in style attribute.";
        }
    }

    // Reload the updated SVG content into the renderer
    renderer->load(svgContent.toUtf8());
}





void LedSuitPictogram::resetColors() {
    for (const QString& partId : partColors.keys()) {
        setPartColor(partId, QColor("black")); // Reset to black
    }
    partColors.clear();
}


void LedSuitPictogram::setAllColors(const SuitState& state) {
    currentColors["head"] = QColor(state.head.r, state.head.g, state.head.b);
    currentColors["bodyPrimary"] = QColor(state.bodyPrimary.r, state.bodyPrimary.g, state.bodyPrimary.b);
    currentColors["bodySecondary"] = QColor(state.bodySecondary.r, state.bodySecondary.g, state.bodySecondary.b);
    currentColors["legPrimary"] = QColor(state.legPrimary.r, state.legPrimary.g, state.legPrimary.b);
    currentColors["legSecondary"] = QColor(state.legSecondary.r, state.legSecondary.g, state.legSecondary.b);
    currentColors["reserve"] = QColor(state.reserve.r, state.reserve.g, state.reserve.b);

    // Apply all colors at once
    applyCurrentColors();
}


void LedSuitPictogram::loadPartBounds() {
    QStringList partIds = {"head", "bodyPrimary", "bodySecondary", "legPrimary", "legSecondary"};
    for (const QString& partId : partIds) {
        if (renderer->elementExists(partId)) {
            QRectF bounds = renderer->boundsOnElement(partId);
            partBounds[partId] = bounds;
        } else {
            qWarning() << "Part ID not found in SVG:" << partId;
        }
    }
}



SuitState LedSuitPictogram::getCurrentState() const {
    SuitState state;

    auto colorToPartState = [](const QColor& color) -> PartState {
        return {static_cast<uint8_t>(color.red()),
                static_cast<uint8_t>(color.green()),
                static_cast<uint8_t>(color.blue())};
    };

    state.head = colorToPartState(currentColors.value("head", QColor("black")));
    state.bodyPrimary = colorToPartState(currentColors.value("bodyPrimary", QColor("black")));
    state.bodySecondary = colorToPartState(currentColors.value("bodySecondary", QColor("black")));
    state.legPrimary = colorToPartState(currentColors.value("legPrimary", QColor("black")));
    state.legSecondary = colorToPartState(currentColors.value("legSecondary", QColor("black")));
    state.reserve = colorToPartState(currentColors.value("reserve", QColor("black")));

    return state;
}

