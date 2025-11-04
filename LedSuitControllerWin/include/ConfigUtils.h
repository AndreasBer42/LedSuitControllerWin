#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <QString>

void ensureConfigFiles();
int loadAppConfig(const QString& configFilePath);

#endif // CONFIG_UTILS_H


