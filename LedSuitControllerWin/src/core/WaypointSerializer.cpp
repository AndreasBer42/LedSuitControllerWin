#include "include/core/WaypointSerializer.h"

void toJson(QJsonObject& json, const SuitState& suitState) {
    QJsonObject head, bodyPrimary, bodySecondary, legPrimary, legSecondary, reserve;

    head["r"] = suitState.head.r;
    head["g"] = suitState.head.g;
    head["b"] = suitState.head.b;

    bodyPrimary["r"] = suitState.bodyPrimary.r;
    bodyPrimary["g"] = suitState.bodyPrimary.g;
    bodyPrimary["b"] = suitState.bodyPrimary.b;

    bodySecondary["r"] = suitState.bodySecondary.r;
    bodySecondary["g"] = suitState.bodySecondary.g;
    bodySecondary["b"] = suitState.bodySecondary.b;

    legPrimary["r"] = suitState.legPrimary.r;
    legPrimary["g"] = suitState.legPrimary.g;
    legPrimary["b"] = suitState.legPrimary.b;

    legSecondary["r"] = suitState.legSecondary.r;
    legSecondary["g"] = suitState.legSecondary.g;
    legSecondary["b"] = suitState.legSecondary.b;

    reserve["r"] = suitState.reserve.r;
    reserve["g"] = suitState.reserve.g;
    reserve["b"] = suitState.reserve.b;

    json["head"] = head;
    json["bodyPrimary"] = bodyPrimary;
    json["bodySecondary"] = bodySecondary;
    json["legPrimary"] = legPrimary;
    json["legSecondary"] = legSecondary;
    json["reserve"] = reserve;
}

void toJson(QJsonObject& json, const Waypoint& waypoint) {
    json["timeInSeconds"] = waypoint.timeInSeconds;

    QJsonArray suitStatesArray;
    for (const auto& suitState : waypoint.suitStates) {
        QJsonObject suitStateJson;
        toJson(suitStateJson, suitState);
        suitStatesArray.append(suitStateJson);
    }
    json["suitStates"] = suitStatesArray;
}


void fromJson(const QJsonObject& json, SuitState& suitState) {
    auto fromPart = [](const QJsonObject& obj, PartState& part) {
        part.r = static_cast<uint8_t>(obj["r"].toInt());
        part.g = static_cast<uint8_t>(obj["g"].toInt());
        part.b = static_cast<uint8_t>(obj["b"].toInt());
    };

    fromPart(json["head"].toObject(), suitState.head);
    fromPart(json["bodyPrimary"].toObject(), suitState.bodyPrimary);
    fromPart(json["bodySecondary"].toObject(), suitState.bodySecondary);
    fromPart(json["legPrimary"].toObject(), suitState.legPrimary);
    fromPart(json["legSecondary"].toObject(), suitState.legSecondary);
    fromPart(json["reserve"].toObject(), suitState.reserve);
}

void fromJson(const QJsonObject& json, Waypoint& waypoint) {
    waypoint.timeInSeconds = json["timeInSeconds"].toDouble();

    QJsonArray suitStatesArray = json["suitStates"].toArray();
    waypoint.suitStates.clear();
    for (const auto& item : suitStatesArray) {
        SuitState suitState;
        fromJson(item.toObject(), suitState);
        waypoint.suitStates.push_back(suitState);
    }
}

QJsonArray serializeWaypoints(const std::vector<Waypoint>& waypoints) {
    qWarning() << "Starting waypoint serialization. Count:" << waypoints.size();

    QJsonArray waypointsArray;
    for (const auto& waypoint : waypoints) {
        QJsonObject waypointJson;
        toJson(waypointJson, waypoint);
        waypointsArray.append(waypointJson);
    }

    qWarning() << "Finished waypoint serialization.";
    return waypointsArray;
}


std::vector<Waypoint> deserializeWaypoints(const QJsonArray& waypointsArray) {
    std::vector<Waypoint> waypoints;

    for (const auto& item : waypointsArray) {
        Waypoint waypoint;
        fromJson(item.toObject(), waypoint);
        waypoints.push_back(waypoint);
    }

    return waypoints;
}


