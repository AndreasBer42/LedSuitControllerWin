#ifndef SUITSTATE_H
#define SUITSTATE_H

#include <cstdint>
#include <vector>

// RGB values for a part
struct PartState {
    uint8_t r, g, b;
};

// RGB values for an entire suit
struct SuitState {
    PartState head;
    PartState bodyPrimary;
    PartState bodySecondary;
    PartState legPrimary;
    PartState legSecondary;
    PartState reserve;
};


struct Waypoint {
    double timeInSeconds;    // Time on the timeline
    std::vector<SuitState> suitStates;   // States of all suits at this time
};



#endif // SUITSTATE_H

