#include <algorithm>
#include <list>
#include <time.h>
#include <windows.h>

#include <QDebug>
#include <QObject>
#include <QString>
#include <QThread>
#include <QtMath>

#include "ftd2xx.h"

#include "common.hpp"
#include "helper.hpp"

#include "workerthread.hpp"

#define PP   0x02
#define READ 0x03
#define RDSR 0x05
#define WREN 0x06
#define INFO 0x9F
#define SE   0xD8

using std::fill;

quint32 WorkerThread::pagesPerSector(quint32 flashsize) {
    return (flashsize == 16777216 ? 262144 : 65536) / PAGESIZE;
}

void WorkerThread::encodeHeader(char instruction, quint16 pageId) {
    Helper::encodeByte(buffer, 0, instruction);
    Helper::encodeByte(buffer, 1, pageId >> 8);
    Helper::encodeByte(buffer, 2, pageId & 0xFF);
    fill(buffer + (8 * 3), buffer + (8 * 4), 0);
}

char WorkerThread::getStatus() {
    char statusBuffer[8*2 + 1];
    Helper::encodeByte(statusBuffer, 0, RDSR);
    fill(statusBuffer + 8, statusBuffer + (8 * 2), 0);
    statusBuffer[8 * 2] = CS;
    FT_Write(handle, &statusBuffer, 8*2 + 1, &bytes);
    FT_Read(handle, &statusBuffer, 8*2 + 1, &bytes);
    return Helper::decodeByte(statusBuffer, 1, 1);
}

void WorkerThread::whileBusy() {
    while (getStatus() & 0x01) {}
}

void WorkerThread::whileReadOnly() {
    while (!(getStatus() & 0x02)) {}
}

bool WorkerThread::writeByte(char byte) {
    char byteBuffer[8 + 1];
    Helper::encodeByte(byteBuffer, 0, byte);
    byteBuffer[8] = CS;
    whileBusy();
    if (
        FT_Write(handle, &byteBuffer, 8 + 1, &bytes) != FT_OK ||
        FT_Read(handle, &byteBuffer, 8 + 1, &bytes) != FT_OK
    ) {
        return false;
    }

    return true;
}

bool WorkerThread::write(char *pageBuffer, quint16 pageId) {
    if (!writeByte(WREN)) {
        return false;
    }

    encodeHeader(PP, pageId);
    whileReadOnly();
    if (
        FT_Write(handle, &buffer, 8 * 4, &bytes) != FT_OK ||
        FT_Write(handle, pageBuffer, PAGEBITS + 1, &bytes) != FT_OK ||
        FT_Read(handle, &buffer, PAGEBITS + 8*4 + 1, &bytes) != FT_OK
    ) {
        return false;
    }

    return true;
}

char *WorkerThread::read(quint16 pageId) {
    char *pageBuffer = new char[PAGEBITS];
    encodeHeader(READ, pageId);
    fill(buffer + (8 * 4), buffer + (PAGEBITS + 8*4), 0);
    buffer[PAGEBITS + 8*4] = CS;
    if (
        FT_Write(handle, &buffer, PAGEBITS + (8*4 + 1), &bytes) != FT_OK ||
        FT_Read(handle, &buffer, (8*4 + 1), &bytes) != FT_OK ||
        FT_Read(handle, pageBuffer, PAGEBITS, &bytes) != FT_OK 
    ) {
        delete[] pageBuffer;
        return NULL;
    }

    return pageBuffer;
}

void WorkerThread::start(std::list<char *> &pages, bool isUpload) {
    emit progress(0);
    this->pages = pages;
    this->pageNumber = pages.size();
    this->isUpload = isUpload;
    start();
}

void WorkerThread::run() {
    clock_t timer = clock();

    FT_PROGRAM_DATA info;

    char ManufacturerBuffer[32];
    char ManufacturerIdBuffer[16];
    char DescriptionBuffer[64];
    char SerialNumberBuffer[16];

    info.Signature1 = 0x00000000;
    info.Signature2 = 0xffffffff;
    info.Version = 0x00000005;
    info.Manufacturer = ManufacturerBuffer;
    info.ManufacturerId = ManufacturerIdBuffer;
    info.Description = DescriptionBuffer;
    info.SerialNumber = SerialNumberBuffer;

    bool found = false;
    for (int i = 0; ; ++i) {
        if (FT_Open(i, &handle) != FT_OK) {
            FT_Close(handle);
            break;
        }

        if (
            FT_EE_Read(handle, &info) == FT_OK &&
            !strcmp(info.Manufacturer, "EKSELCOM")
        ) {
            found = true;
            break;
        }

        FT_Close(handle);
    }

    if (!found) {
        emit status("ERROR: no device found");
        emit buttons(Buttons::Step2);
        return;
    }

    if (
        FT_SetUSBParameters(handle, 0x10000, 0) != FT_OK ||
        FT_SetBitMode(handle, MOSI | CS, FT_BITMODE_SYNC_BITBANG) != FT_OK ||
        FT_SetDivisor(handle, 0) != FT_OK ||
        FT_SetLatencyTimer(handle, 2) != FT_OK
    ) {
        FT_Close(handle);
        emit status("ERROR: device initialization failed");
        emit buttons(Buttons::Step2);
        return;
    }

    QString error = "";
    quint32 pps = 1;
    Helper::encodeByte(buffer, 0, 0x9F);
    fill(buffer + 8, buffer + (8 * 4), 0);
    buffer[8 * 4] = CS;
    if (
        FT_Write(handle, &buffer, 8*4 + 1, &bytes) != FT_OK ||
        FT_Read(handle, &buffer, 8*4 + 1, &bytes) != FT_OK ||
        Helper::decodeByte(buffer, 1, 1) != 0x20 ||
        Helper::decodeByte(buffer, 2, 1) != 0x20
    ) {
        error = "ERROR: no compatible flash connected";
    }
    else {
        quint32 flashsize = qPow(2, Helper::decodeByte(buffer, 3, 1));
        quint32 datasize = pageNumber * PAGESIZE;
        pps = pagesPerSector(flashsize);
        if (datasize > flashsize) {
            error = "ERROR: won't fit (flash is " +
                QString::number(qCeil(flashsize / 1024.0)) + " KiB, data is " +
                QString::number(qCeil(datasize / 1024.0)) + " KiB";
        }
    }

    if (!error.isEmpty()) {
        FT_Close(handle);
        emit status(error);
        emit buttons(Buttons::Step2);
        return;
    }

    emit status("BUSY: " + QString(isUpload ? "upload" : "verify") + "ing...");

    bool success = true;
    auto page = pages.begin();
    for (int i = 0; i < pageNumber; ++i, ++page) {
        if (isUpload) {
            if (!(i % pps)) {
                if (!writeByte(WREN)) {
                    success = false;
                    break;
                }

                encodeHeader(SE, i);
                buffer[8 * 4] = CS;
                whileReadOnly();
                if (
                    FT_Write(handle, &buffer, 8*4 + 1, &bytes) != FT_OK ||
                    FT_Read(handle, &buffer, 8*4 + 1, &bytes) != FT_OK
                ) {
                    success = false;
                    break;
                }
            }
            if (!write(*page, i)) {
                success = false;
                break;
            }
        }
        else {
            char *pageBuffer = read(i);
            if (NULL == pageBuffer) {
                success = false;
                break;
            }

            for (int j = 0; j < PAGEBITS; ++j) {
                if (
                    (((*page)[j] > 0) && !(pageBuffer[j] & MISO)) ||
                    (!(*page)[j] && (pageBuffer[j] & MISO))
                ) {
                    success = false;
                    break;
                }
            }

            delete[] pageBuffer;
            if (!success) {
                break;
            }
        }

        emit progress(qFloor((double)((i + 1) * 10000) / pageNumber));
    }

    FT_Close(handle);
    if (success) {
        emit status(
            "SUCCESS: " + QString(isUpload ? "upload" : "verifi") + "ed in " +
            QString::number((double)(clock() - timer) / CLOCKS_PER_SEC, 'f', 3)
            + "s"
        );
    }
    else {
        emit status(
            "ERROR: " + QString(isUpload ?
                "upload failed, try again" :
                "verification failed, try reuploading"
            )
        );
    }

    emit buttons(Buttons::Step2);
}
