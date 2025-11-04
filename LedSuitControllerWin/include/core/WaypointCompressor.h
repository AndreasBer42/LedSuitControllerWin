#ifndef WAYPOINT_COMPRESSOR_H
#define WAYPOINT_COMPRESSOR_H

#include "include/ui/SpectrogramView.h"
#include <cstdint>
#include <vector>
#include <memory>

class WaypointCompressor {
public:
    WaypointCompressor(SpectrogramView* spectrogramView);

    // Method to compress waypoints into vectors for each suit
    std::vector<std::vector<uint8_t>> compressWaypoints() const;

private:
    SpectrogramView* spectrogramView;

    // Helper to compress a single SuitState into a byte
    uint8_t compressSuitState(const SuitState& state) const;

    // Helper to convert time in seconds to milliseconds as a 4-byte vector
    std::vector<uint8_t> timeToBytes(double timeInSeconds) const;
};

#endif // WAYPOINT_COMPRESSOR_H

