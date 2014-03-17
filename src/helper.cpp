#include <QtGlobal>

#include "common.hpp"

#include "helper.hpp"

void Helper::encodeByte(
    char *buffer, quint16 position, char byte, quint8 positionOffset
) {
    position = position*8 + positionOffset + 8;
    for (int i = 0; i < 8; ++i) {
        buffer[--position] = MOSI * (byte & 0x1);
        byte >>= 1;
    }
}

char Helper::decodeByte(
    char *buffer, quint16 position, quint8 positionOffset
) {
    char result = 0;
    quint8 power = 128;
    position = position*8 + positionOffset;
    for (int i = 0; i < 8; ++i) {
        result += power * ((buffer[position + i] & MISO) / MISO);
        power >>= 1;
    }

    return result;
}
