#ifndef VERSION_MANAGER_H
#define VERSION_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "version_config.h"

class VersionManager
{
public:
    static VersionManager &getInstance();

    // Getters for version information
    const char *getVersion() const { return version; }
    const char *getBuildDate() const { return builddate; }
    const char *getDevice() const { return device; }

    // Method to populate JSON document with version info
    void populateVersionInfo(JsonDocument &doc) const;

private:
    VersionManager() = default;
    ~VersionManager() = default;
    VersionManager(const VersionManager &) = delete;
    VersionManager &operator=(const VersionManager &) = delete;
};

extern VersionManager &versionManager;

#endif // VERSION_MANAGER_H