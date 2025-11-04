#ifndef TIMELINE_H
#define TIMELINE_H

#include <vector>
#include <map>
#include <cstdint>
#include <string>
#include "include/core/SuitState.h"


// A timeline entry
struct TimelineEntry {
    double timestamp;
    std::vector<SuitState> suits;
};

class Timeline {
public:
    Timeline();
    ~Timeline();

    void addTimePoint(double time, const std::vector<SuitState>& suits);
    void removeTimePoint(double time);
    void editTimePoint(double time, const std::vector<SuitState>& suits);
    std::vector<SuitState> getStateAtTime(double time) const;

    void loadFromFile(const std::string& filePath);
    void saveToFile(const std::string& filePath) const;

    const std::map<double, std::vector<SuitState>>& getTimeline() const { return timeline; }
    void setTimeline(const std::map<double, std::vector<SuitState>>& newTimeline) { timeline = newTimeline; }

private:
    std::map<double, std::vector<SuitState>> timeline;
};

#endif // TIMELINE_H

