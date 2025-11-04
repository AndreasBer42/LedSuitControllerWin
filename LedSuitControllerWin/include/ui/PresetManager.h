#ifndef PRESET_MANAGER_H
#define PRESET_MANAGER_H

#include <vector>
#include <string>
#include "include/ui/Preset.h"

class PresetManager {
public:
    PresetManager();
    ~PresetManager();

    void addPreset(const Preset& preset);
    void removePreset(const std::string& name);
    void editPreset(const std::string& name, const Preset& newPreset);
    const Preset* getPreset(const std::string& name) const;
    const std::vector<Preset>& getPresets() const;

    void loadFromFile(const std::string& filePath);
    void saveToFile(const std::string& filePath) const;
    void saveCurrentState(const std::string& presetName, const std::vector<SuitState>& suitStates, const std::string& filePath);


private:
    std::vector<Preset> presets; // List of presets
};

#endif // PRESET_MANAGER_H

