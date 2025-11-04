#ifndef WAYPOINTSERIALIZER_H
#define WAYPOINTSERIALIZER_H

#include "include/core/SuitState.h"
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

// Serialization functions
void toJson(QJsonObject& json, const SuitState& suitState);
void toJson(QJsonObject& json, const Waypoint& waypoint);

QJsonArray serializeWaypoints(const std::vector<Waypoint>& waypoints);


// Deserialization functions
void fromJson(const QJsonObject& json, SuitState& suitState);
void fromJson(const QJsonObject& json, Waypoint& waypoint);
std::vector<Waypoint> deserializeWaypoints(const QJsonArray& waypointsArray);


#endif // WAYPOINTSERIALIZER_H

