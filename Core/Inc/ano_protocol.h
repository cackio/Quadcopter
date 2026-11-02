#ifndef ANO_PROTOCOL_H
#define ANO_PROTOCOL_H
#include "flight_app.h"
#include <stddef.h>
#include <stdint.h>
uint16_t ANO_Pack(uint8_t id, const uint8_t *data, uint8_t len, uint8_t *out, size_t capacity);
uint16_t ANO_PackEuler(const float degrees[3], uint8_t *out, size_t capacity);
uint16_t ANO_PackQuaternion(const float q[4], uint8_t *out, size_t capacity);
void ANO_Put16(uint8_t *out, uint16_t value);
void ANO_Put32(uint8_t *out, uint32_t value);
int16_t ANO_Scale(float value, float scale);
uint16_t ANO_PackDiagnostic(const FlightSnapshot *s, uint8_t *out, size_t capacity);
#endif
