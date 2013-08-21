#ifndef HELPER
#define HELPER

#include <QtCore/QtGlobal>

#define MISO    0x20 // DSR <
#define MOSI    0x02 // RxD >

class Helper
{
public:
    static void encodeByte(
        char *pageBuffer, quint16 position, char byte, quint8 positionOffset = 0
    );
    static char decodeByte(
        char *pageBuffer, quint16 position, quint8 positionOffset = 0
    );
};

#endif
