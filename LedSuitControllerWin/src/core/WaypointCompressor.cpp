#include "include/core/WaypointCompressor.h"
#include <cmath>
#include <iostream> // For debugging (optional)
#include <algorithm> // For std::min

WaypointCompressor::WaypointCompressor(SpectrogramView* spectrogramView)
    : spectrogramView(spectrogramView) {}

std::vector<std::vector<uint8_t>> WaypointCompressor::compressWaypoints() const {
    if (!spectrogramView) {
        throw std::runtime_error("SpectrogramView is null.");
    }

    const auto& waypoints = spectrogramView->getWaypoints();

    if (waypoints.empty()) {
        std::cerr << "No waypoints to compress." << std::endl;
        return {};
    }

    // Determine the number of suits
    const size_t numSuits = waypoints.front()->suitStates.size();

    // Initialize output: one vector per suit
    std::vector<std::vector<uint8_t>> compressedData(numSuits);

    // Compress each waypoint
    for (const auto& waypoint : waypoints) {
        double timeInSeconds = waypoint->timeInSeconds;

        // Convert time to a 4-byte representation
        std::vector<uint8_t> timeBytes = timeToBytes(timeInSeconds);

        for (size_t suitIndex = 0; suitIndex < numSuits; ++suitIndex) {
            // Compress the suit state into a single byte
            uint8_t stateByte = compressSuitState(waypoint->suitStates[suitIndex]);

            // Append the compressed data to the corresponding suit's vector
            compressedData[suitIndex].insert(compressedData[suitIndex].end(), timeBytes.begin(), timeBytes.end());
            compressedData[suitIndex].push_back(stateByte);
        }
    }
    return compressedData;
}

uint8_t WaypointCompressor::compressSuitState(const SuitState& state) const {
    uint8_t result = 0;

    // Encode each part as a bit
    result |= (state.head.r || state.head.g || state.head.b) << 5;
    result |= (state.bodyPrimary.r || state.bodyPrimary.g || state.bodyPrimary.b) << 4;
    result |= (state.bodySecondary.r || state.bodySecondary.g || state.bodySecondary.b) << 3;
    result |= (state.legPrimary.r || state.legPrimary.g || state.legPrimary.b) << 2;
    result |= (state.legSecondary.r || state.legSecondary.g || state.legSecondary.b) << 1;
    result |= (state.reserve.r || state.reserve.g || state.reserve.b);

    return result;
}

std::vector<uint8_t> WaypointCompressor::timeToBytes(double timeInSeconds) const {
    uint32_t timeInMilliseconds = static_cast<uint32_t>(std::round(timeInSeconds * 1000));
    return {
        static_cast<uint8_t>((timeInMilliseconds >> 24) & 0xFF),
        static_cast<uint8_t>((timeInMilliseconds >> 16) & 0xFF),
        static_cast<uint8_t>((timeInMilliseconds >> 8) & 0xFF),
        static_cast<uint8_t>(timeInMilliseconds & 0xFF)
    };
}

