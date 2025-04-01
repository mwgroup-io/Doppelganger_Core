#include "version_config.h"

// Version info
const char *version = "2.0.0";
const char *builddate = "22MAR2025";
const char *device = "Doppelganger Pro";
const char *hardware = "1.0a";

// Debug output macros
bool DEBUG_ENABLED = true;

// Hardware GPIO pins
const int C_PIN_LED = 11; // GPIO for the LED (OUTPUT)

// WiFiManager Configurations
const char *defaultPASS = "UndertheRadar";
const char *prefixSSID = "doppelgänger_"; // Using UTF-8 encoded ä