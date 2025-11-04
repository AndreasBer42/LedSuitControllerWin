#ifndef PRESET_H
#define PRESET_H

#include <vector>
#include <string>
#include "include/core/Timeline.h" // For SuitState

class Preset {
public:
    Preset(const std::string& name, const std::vector<SuitState>& suits);
    ~Preset();

    const std::string& getName() const;
    void setName(const std::string& newName);

    const std::vector<SuitState>& getSuitStates() const;
    void setSuitStates(const std::vector<SuitState>& suits);

private:
    std::string name;               // Preset name
    std::vector<SuitState> suits;   // Suit states for the preset
};

#endif // PRESET_H


