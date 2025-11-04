#include "include/core/Timeline.h"
#include <stdexcept>
#include <iostream>
#include "include/core/JSONHandler.h"

Timeline::Timeline() {}

Timeline::~Timeline() {}

void Timeline::addTimePoint(double time, const std::vector<SuitState>& suits) {
    timeline[time] = suits;
    std::cout << "Added time point at: " << time << " seconds." << std::endl;
}

void Timeline::removeTimePoint(double time) {
    if (timeline.erase(time) > 0) {
        std::cout << "Removed time point at: " << time << " seconds." << std::endl;
    } else {
        throw std::runtime_error("Time point not found.");
    }
}

void Timeline::editTimePoint(double time, const std::vector<SuitState>& suits) {
    if (timeline.find(time) != timeline.end()) {
        timeline[time] = suits;
        std::cout << "Edited time point at: " << time << " seconds." << std::endl;
    } else {
        throw std::runtime_error("Time point not found.");
    }
}

std::vector<SuitState> Timeline::getStateAtTime(double time) const {
    auto it = timeline.lower_bound(time);
    if (it != timeline.end()) {
        return it->second;
    }
    throw std::runtime_error("No state found for the specified time.");
}

void Timeline::loadFromFile(const std::string& filePath) {
    JSONHandler::loadTimeline(*this, filePath);
}

void Timeline::saveToFile(const std::string& filePath) const {
    JSONHandler::saveTimeline(*this, filePath);
}

