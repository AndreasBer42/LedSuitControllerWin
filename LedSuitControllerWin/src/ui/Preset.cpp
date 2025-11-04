#include "include/ui/Preset.h"

Preset::Preset(const std::string& name, const std::vector<SuitState>& suits)
    : name(name), suits(suits) {}

Preset::~Preset() {}

const std::string& Preset::getName() const {
    return name;
}

void Preset::setName(const std::string& newName) {
    name = newName;
}

const std::vector<SuitState>& Preset::getSuitStates() const {
    return suits;
}

void Preset::setSuitStates(const std::vector<SuitState>& suits) {
    this->suits = suits;
}

