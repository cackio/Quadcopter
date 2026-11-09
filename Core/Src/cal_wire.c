#include "cal_wire.h"
#include "ano_protocol.h"
#include <string.h>
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
int CalParser_Feed(CalParser *p, uint8_t byte, CalCommand *cmd)
{
    unsigned i;
    uint8_t sc = 0, ac = 0;
    p->bytes[p->used++] = byte;
    while (p->used)
    {
        if (p->bytes[0] != 0xAA || (p->used >= 2 && p->bytes[1] != 0xFF) ||
            (p->used >= 3 && p->bytes[2] != SCAL_CMD_ID) ||
            (p->used >= 4 && p->bytes[3] != SCAL_CMD_SIZE))
        {
            memmove(p->bytes, p->bytes + 1, --p->used);
            continue;
        }
        if (p->used < 18)
            return 0;
        for (i = 0; i < 16; ++i)
        {
            sc = (uint8_t)(sc + p->bytes[i]);
            ac = (uint8_t)(ac + sc);
        }
        if (sc != p->bytes[16] || ac != p->bytes[17] || p->bytes[4] != SCAL_VERSION ||
            p->bytes[7] || p->bytes[14] || p->bytes[15])
        {
            ++p->bad_frames;
            memmove(p->bytes, p->bytes + 1, --p->used);
            continue;
        }
        cmd->op = p->bytes[5];
        cmd->face = p->bytes[6];
        cmd->id = get32(p->bytes + 8);
        cmd->samples = (uint16_t)(p->bytes[12] | (uint16_t)p->bytes[13] << 8);
        p->used = 0;
        return 1;
    }
    return 0;
}
void CalWire_Float(uint8_t *out, float value)
{
    uint32_t bits;
    memcpy(&bits, &value, 4);
    ANO_Put32(out, bits);
}
void CalWire_Header(uint8_t data[84], const CalCommand *cmd, uint8_t state, uint8_t reason)
{
    memset(data, 0, SCAL_REPLY_SIZE);
    data[0] = SCAL_VERSION;
    data[1] = cmd->op;
    data[2] = cmd->face;
    data[3] = state;
    ANO_Put32(data + 4, cmd->id);
    data[8] = reason;
}
uint16_t CalWire_Result(const CalService *s, uint8_t out[96])
{
    uint8_t data[84];
    unsigned i;
    CalCommand cmd = {s->id, s->op, s->face, s->target};
    CalWire_Header(data, &cmd, s->state, s->reason);
    data[9] = s->last_reason;
    ANO_Put16(data + 10, (uint16_t)s->moments.count);
    ANO_Put16(data + 12, s->target);
    ANO_Put16(data + 14, s->resets);
    ANO_Put32(data + 16, s->first_sequence);
    ANO_Put32(data + 20, s->last_sequence);
    ANO_Put32(data + 24, s->duration_us);
    for (i = 0; i < 6; ++i)
    {
        CalWire_Float(data + 28 + 4 * i, s->moments.mean[i]);
        CalWire_Float(data + 52 + 4 * i, CalMoments_Std(&s->moments, i));
    }
    CalWire_Float(data + 76, s->moments.mean[6] / 340.0f + 36.53f);
    ANO_Put32(data + 80, s->dt_bad);
    return ANO_Pack(SCAL_REPLY_ID, data, sizeof(data), out, 96);
}
