#include "include/ui/PresetManager.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <QFile>            // For QFile
#include <QJsonDocument>    // For QJsonDocument
#include <QJsonObject>      // For QJsonObject
#include <QJsonArray>       // For QJsonArray
#include <QString>          // For QString

PresetManager::PresetManager() {}

PresetManager::~PresetManager() {}

void PresetManager::addPreset(const Preset& preset) {
    presets.push_back(preset);
    std::cout << "Added preset: " << preset.getName() << std::endl;
}

void PresetManager::removePreset(const std::string& name) {
    auto it = std::remove_if(presets.begin(), presets.end(),
        [&name](const Preset& p) { return p.getName() == name; });
    if (it != presets.end()) {
        presets.erase(it, presets.end());
        std::cout << "Removed preset: " << name << std::endl;
    } else {
        throw std::runtime_error("Preset not found: " + name);
    }
}

void PresetManager::editPreset(const std::string& name, const Preset& newPreset) {
    for (auto& preset : presets) {
        if (preset.getName() == name) {
            preset = newPreset;
            std::cout << "Edited preset: " << name << std::endl;
            return;
        }
    }
    throw std::runtime_error("Preset not found: " + name);
}

const Preset* PresetManager::getPreset(const std::string& name) const {
    for (const auto& preset : presets) {
        if (preset.getName() == name) {
            return &preset;
        }
    }
    return nullptr;
}

const std::vector<Preset>& PresetManager::getPresets() const {
    return presets;
}

void PresetManager::loadFromFile(const std::string& filePath) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << QString::fromStdString(filePath);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray presetArray = doc.array();
    presets.clear();

    for (const QJsonValue& value : presetArray) {
        QJsonObject presetObj = value.toObject();

        std::string name = presetObj["name"].toString().toStdString();
        QJsonArray suitsArray = presetObj["suits"].toArray();

        std::vector<SuitState> suits;

        // Expect exactly 8 suit states
        for (const QJsonValue& suitValue : suitsArray) {
            QJsonObject suitObj = suitValue.toObject();
            SuitState state;

            state.head = {static_cast<uint8_t>(suitObj["head_r"].toInt()),
                          static_cast<uint8_t>(suitObj["head_g"].toInt()),
                          static_cast<uint8_t>(suitObj["head_b"].toInt())};

            state.bodyPrimary = {static_cast<uint8_t>(suitObj["bodyPrimary_r"].toInt()),
                                 static_cast<uint8_t>(suitObj["bodyPrimary_g"].toInt()),
                                 static_cast<uint8_t>(suitObj["bodyPrimary_b"].toInt())};

            state.bodySecondary = {static_cast<uint8_t>(suitObj["bodySecondary_r"].toInt()),
                                   static_cast<uint8_t>(suitObj["bodySecondary_g"].toInt()),
                                   static_cast<uint8_t>(suitObj["bodySecondary_b"].toInt())};

            state.legPrimary = {static_cast<uint8_t>(suitObj["legPrimary_r"].toInt()),
                                static_cast<uint8_t>(suitObj["legPrimary_g"].toInt()),
                                static_cast<uint8_t>(suitObj["legPrimary_b"].toInt())};

            state.legSecondary = {static_cast<uint8_t>(suitObj["legSecondary_r"].toInt()),
                                  static_cast<uint8_t>(suitObj["legSecondary_g"].toInt()),
                                  static_cast<uint8_t>(suitObj["legSecondary_b"].toInt())};

            state.reserve = {static_cast<uint8_t>(suitObj["reserve_r"].toInt()),
                             static_cast<uint8_t>(suitObj["reserve_g"].toInt()),
                             static_cast<uint8_t>(suitObj["reserve_b"].toInt())};

            suits.push_back(state);
        }

        if (suits.size() == 8) {
            presets.emplace_back(name, suits);
            qDebug() << "Loaded preset:" << QString::fromStdString(name) << "with 8 suits.";
        } else {
            qWarning() << "Invalid number of suits in preset:" << QString::fromStdString(name);
        }
    }

    file.close();
}


void PresetManager::saveCurrentState(const std::string& presetName, const std::vector<SuitState>& suitStates, const std::string& filePath) {
    QJsonArray presetArray;

    // Load existing presets first
    QFile file(QString::fromStdString(filePath));
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        presetArray = doc.array();
        file.close();
    } else {
        qWarning() << "Failed to open file for reading:" << QString::fromStdString(filePath);
    }

    // Convert the new state to JSON
    QJsonObject newPreset;
    newPreset["name"] = QString::fromStdString(presetName);

    QJsonArray suitsArray;
    for (const auto& state : suitStates) {
        QJsonObject suitObj;
        suitObj["head_r"] = state.head.r;
        suitObj["head_g"] = state.head.g;
        suitObj["head_b"] = state.head.b;

        suitObj["bodyPrimary_r"] = state.bodyPrimary.r;
        suitObj["bodyPrimary_g"] = state.bodyPrimary.g;
        suitObj["bodyPrimary_b"] = state.bodyPrimary.b;

        suitObj["bodySecondary_r"] = state.bodySecondary.r;
        suitObj["bodySecondary_g"] = state.bodySecondary.g;
        suitObj["bodySecondary_b"] = state.bodySecondary.b;

        suitObj["legPrimary_r"] = state.legPrimary.r;
        suitObj["legPrimary_g"] = state.legPrimary.g;
        suitObj["legPrimary_b"] = state.legPrimary.b;

        suitObj["legSecondary_r"] = state.legSecondary.r;
        suitObj["legSecondary_g"] = state.legSecondary.g;
        suitObj["legSecondary_b"] = state.legSecondary.b;

        suitObj["reserve_r"] = state.reserve.r;
        suitObj["reserve_g"] = state.reserve.g;
        suitObj["reserve_b"] = state.reserve.b;

        suitsArray.append(suitObj);
    }

    newPreset["suits"] = suitsArray;
    presetArray.append(newPreset);

    // Save back to file
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(presetArray);
        file.write(doc.toJson());
        file.close();
        qDebug() << "Saved current state to file as preset:" << QString::fromStdString(presetName);
    } else {
        qWarning() << "Failed to open file for writing:" << QString::fromStdString(filePath);
    }
}



void PresetManager::saveToFile(const std::string& filePath) const {
    QJsonArray presetArray;

    for (const Preset& preset : presets) {
        QJsonObject presetObj;
        presetObj["name"] = QString::fromStdString(preset.getName());

        QJsonArray suitsArray;
        for (const SuitState& suit : preset.getSuitStates()) {
            QJsonObject suitObj;

            // Save all six parts for the current suit
            suitObj["head_r"] = suit.head.r;
            suitObj["head_g"] = suit.head.g;
            suitObj["head_b"] = suit.head.b;

            suitObj["bodyPrimary_r"] = suit.bodyPrimary.r;
            suitObj["bodyPrimary_g"] = suit.bodyPrimary.g;
            suitObj["bodyPrimary_b"] = suit.bodyPrimary.b;

            suitObj["bodySecondary_r"] = suit.bodySecondary.r;
            suitObj["bodySecondary_g"] = suit.bodySecondary.g;
            suitObj["bodySecondary_b"] = suit.bodySecondary.b;

            suitObj["legPrimary_r"] = suit.legPrimary.r;
            suitObj["legPrimary_g"] = suit.legPrimary.g;
            suitObj["legPrimary_b"] = suit.legPrimary.b;

            suitObj["legSecondary_r"] = suit.legSecondary.r;
            suitObj["legSecondary_g"] = suit.legSecondary.g;
            suitObj["legSecondary_b"] = suit.legSecondary.b;

            suitObj["reserve_r"] = suit.reserve.r;
            suitObj["reserve_g"] = suit.reserve.g;
            suitObj["reserve_b"] = suit.reserve.b;

            suitsArray.append(suitObj);
        }

        presetObj["suits"] = suitsArray;
        presetArray.append(presetObj);
    }

    QJsonDocument doc(presetArray);
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::WriteOnly)) {
        std::cerr << "Failed to save file: " << filePath << std::endl;
        return;
    }
    file.write(doc.toJson());
    file.close();
}


