#include "include/core/JSONHandler.h"
#include "include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

void JSONHandler::saveTimeline(const Timeline& timeline, const std::string& filePath) {
    json j;
    j["timeline"] = json::array();

    for (const auto& [time, suits] : timeline.getTimeline()) {
        json timePoint;
        timePoint["time"] = time;

        for (const auto& suit : suits) {
            json suitJson = {
                {"head", {suit.head.r, suit.head.g, suit.head.b}},
                {"bodyPrimary", {suit.bodyPrimary.r, suit.bodyPrimary.g, suit.bodyPrimary.b}},
                {"bodySecondary", {suit.bodySecondary.r, suit.bodySecondary.g, suit.bodySecondary.b}},
                {"legPrimary", {suit.legPrimary.r, suit.legPrimary.g, suit.legPrimary.b}},
                {"legSecondary", {suit.legSecondary.r, suit.legSecondary.g, suit.legSecondary.b}},
                {"reserve", {suit.reserve.r, suit.reserve.g, suit.reserve.b}}
            };
            timePoint["suits"].push_back(suitJson);
        }

        j["timeline"].push_back(timePoint);
    }

    std::ofstream outFile(filePath);
    if (!outFile) {
        throw std::runtime_error("Failed to open file for writing: " + filePath);
    }
    outFile << j.dump(4);
    std::cout << "Saved timeline to " << filePath << std::endl;
}

void JSONHandler::loadTimeline(Timeline& timeline, const std::string& filePath) {
    json j;

    std::ifstream inFile(filePath);
    if (!inFile) {
        throw std::runtime_error("Failed to open file for reading: " + filePath);
    }
    inFile >> j;

    std::map<double, std::vector<SuitState>> newTimeline;

    for (const auto& timePoint : j["timeline"]) {
        double time = timePoint["time"];
        std::vector<SuitState> suits;

        for (const auto& suitJson : timePoint["suits"]) {
            SuitState suit = {
                {suitJson["head"][0], suitJson["head"][1], suitJson["head"][2]},
                {suitJson["bodyPrimary"][0], suitJson["bodyPrimary"][1], suitJson["bodyPrimary"][2]},
                {suitJson["bodySecondary"][0], suitJson["bodySecondary"][1], suitJson["bodySecondary"][2]},
                {suitJson["legPrimary"][0], suitJson["legPrimary"][1], suitJson["legPrimary"][2]},
                {suitJson["legSecondary"][0], suitJson["legSecondary"][1], suitJson["legSecondary"][2]},
                {suitJson["reserve"][0], suitJson["reserve"][1], suitJson["reserve"][2]}
            };
            suits.push_back(suit);
        }

        newTimeline[time] = suits;
    }

    timeline.setTimeline(newTimeline);
    std::cout << "Loaded timeline from " << filePath << std::endl;
}


