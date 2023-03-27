#pragma once

#include <stdint.h>
#include <string>

#define WEBFT8_FT8_TONES_NN 79

typedef struct WebFt8Tones {
    bool ok;
    uint8_t tones[WEBFT8_FT8_TONES_NN];
} WebFt8Tones;

WebFt8Tones pack_telemetry(const char* message);
std::string pack2json(const char* message); // throws std::runtime_exception on error.