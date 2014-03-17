#pragma once

#include <QtGlobal>

class Helper
{
public:
    static void encodeByte(
        char *buffer, quint16 position, char byte, quint8 positionOffset = 0
    );
    static char decodeByte(
        char *buffer, quint16 position, quint8 positionOffset = 0
    );
};
