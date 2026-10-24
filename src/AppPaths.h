#pragma once
#include <QDir>
#include <QStandardPaths>

// Keep application data separate without hiding the desktop theme configuration.
inline QString appConfigDirectory() {
 const auto overridePath=qEnvironmentVariable("GAZEGRID_CONFIG_DIR");
 return overridePath.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation):overridePath;
}
