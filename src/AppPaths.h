#pragma once
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>

// Keep application data separate without hiding the desktop theme configuration.
inline QString appConfigDirectory() {
 const auto overridePath=qEnvironmentVariable("GAZEGRID_CONFIG_DIR");
 return overridePath.isEmpty()?QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation):overridePath;
}

inline QString keyboardPackDirectory() {
 const auto installed=QCoreApplication::applicationDirPath()+"/keyboards";
 if(QDir(installed).exists())return installed;
#ifdef DEFAULT_PACK_PATH
 return QStringLiteral(DEFAULT_PACK_PATH);
#else
 return installed;
#endif
}
