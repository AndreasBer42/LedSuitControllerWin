#ifndef JSONHANDLER_H
#define JSONHANDLER_H

#include "include/core/Timeline.h"
#include <string>

class JSONHandler {
public:
    static void saveTimeline(const Timeline& timeline, const std::string& filePath);
    static void loadTimeline(Timeline& timeline, const std::string& filePath);
};

#endif // JSONHANDLER_H

