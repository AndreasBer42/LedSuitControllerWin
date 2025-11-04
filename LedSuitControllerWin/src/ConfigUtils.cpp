#include "include/ConfigUtils.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>


const int DefaultNumSuits = 8;
// Default values
namespace {
    const QString DefaultConfigDir = QDir::currentPath() + "/config";
    const QString DefaultAppConfigPath = DefaultConfigDir + "/appconfig.json";
    const QString DefaultSuitsConfigPath = DefaultConfigDir + "/suits.json";
}

// Helper function to write JSON data to a file
static bool writeJsonToFile(const QString& filePath, const QJsonDocument& jsonDoc) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    file.write(jsonDoc.toJson());
    file.close();
    return true;
}

static void createDefaultAppConfig(const QString& configFilePath) {
    QJsonObject defaultConfig;
    defaultConfig["numSuits"] = ::DefaultNumSuits; // Use fully qualified name

    if (writeJsonToFile(configFilePath, QJsonDocument(defaultConfig))) {
        qDebug() << "Created default appconfig.json at:" << configFilePath;
    } else {
        qWarning() << "Failed to create appconfig.json at:" << configFilePath;
    }
}

static void createDefaultSuitConfig(const QString& configFilePath) {
    QJsonArray defaultSuits;
    for (int i = 0; i < ::DefaultNumSuits; ++i) { // Fully qualified name
        QJsonObject suit;
        suit["name"] = QString("Suit %1").arg(i + 1);
        suit["ip"] = "";
        QJsonObject color;
        color["red"] = true;
        color["green"] = false;
        suit["color"] = color;
        defaultSuits.append(suit);
    }

    if (writeJsonToFile(configFilePath, QJsonDocument(defaultSuits))) {
        qDebug() << "Created default suits.json at:" << configFilePath;
    } else {
        qWarning() << "Failed to create suits.json at:" << configFilePath;
    }
}


// Public function implementations
void ensureConfigFiles() {
    // Ensure the config directory exists
    QDir configDir(DefaultConfigDir);
    if (!configDir.exists() && !configDir.mkpath(".")) {
        qWarning() << "Failed to create config directory at:" << DefaultConfigDir;
        return;
    }

    // Create appconfig.json if it doesn't exist
    if (!QFile::exists(DefaultAppConfigPath)) {
        createDefaultAppConfig(DefaultAppConfigPath);
    }

    // Create suits.json if it doesn't exist
    if (!QFile::exists(DefaultSuitsConfigPath)) {
        createDefaultSuitConfig(DefaultSuitsConfigPath);
    }
}

