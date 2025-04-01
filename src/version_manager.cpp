#include "version_manager.h"

// Global instance
VersionManager &versionManager = VersionManager::getInstance();

VersionManager &VersionManager::getInstance()
{
    static VersionManager instance;
    return instance;
}

void VersionManager::populateVersionInfo(JsonDocument &doc) const
{
    doc["version"] = version;
    doc["buildDate"] = builddate;
    doc["device"] = device;
}