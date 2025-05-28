#include "version_config.h"

// Version info
const char *version = "1.0.1";
const char *builddate = "20250528";
const char *device = "Core";
const char *hardware = "1.0a";

// Debug output macros
bool DEBUG_ENABLED = true;

// Hardware GPIO pins
const int C_PIN_LED = 11; // GPIO for the LED (OUTPUT)

// WiFiManager Configurations
const char *defaultPASS = "UndertheRadar";
const char *prefixSSID = "doppelganger_";
