#include "workerthread.hpp"

char WorkerThread::getStatus(FT_HANDLE handle)
{
    Helper::encodeByte(buffer, 0, STATUS_READ);
    Helper::encodeByte(buffer, 1, STATUS_READ);
    buffer[16] = CS;
    FT_Write(handle, &buffer, 17, &bytes);
    FT_Read(handle, &buffer, 17, &bytes);
    return Helper::decodeByte(buffer, 1, 1);
}

bool WorkerThread::isValid(FT_HANDLE handle)
{
    return (getStatus(handle) & 0x3C) == 0x2C;
}

bool WorkerThread::isReady(FT_HANDLE handle)
{
    return getStatus(handle) & 0x80;
}

char* WorkerThread::read(FT_HANDLE handle, quint16 pageId)
{
    char *pageBuffer = new char[4224];
    Helper::encodeByte(buffer, 0, MAIN_PAGE_READ);
    Helper::encodeByte(buffer, 1, (char)((pageId & 0xFC0) >> 6));
    Helper::encodeByte(buffer, 2, (char)((pageId & 0x3F) << 2));
    std::fill(buffer + 24, buffer + 4289, 0);
    buffer[4288] = CS;
    FT_Write(handle, &buffer, 4289, &bytes);
    FT_Read(handle, &buffer, 65, &bytes);
    FT_Read(handle, pageBuffer, 4224, &bytes);
    return pageBuffer;
}

bool WorkerThread::write(
    FT_HANDLE handle, bool firstBuffer, char *pageBuffer, quint16 pageId
)
{
    char BUFFER_WRITE = firstBuffer ? BUFFER1_WRITE : BUFFER2_WRITE;
    char BUFFER_FLUSH = firstBuffer ? BUFFER1_FLUSH : BUFFER2_FLUSH;
    int status = 0;

    Helper::encodeByte(buffer, 0, BUFFER_WRITE);
    std::fill(buffer + 8, buffer + 32, 0);
    status += FT_Write(handle, &buffer, 32, &bytes);
    status += FT_Write(handle, pageBuffer, 4225, &bytes);
    status += FT_Read(handle, &buffer, 4257, &bytes);
    while (!isReady(handle)) {}

    Helper::encodeByte(buffer, 0, BUFFER_FLUSH);
    Helper::encodeByte(buffer, 1, (char)((pageId & 0xFC0) >> 6));
    Helper::encodeByte(buffer, 2, (char)((pageId & 0x3F) << 2));
    std::fill(buffer + 24, buffer + 32, 0);
    buffer[32] = CS;
    status += FT_Write(handle, &buffer, 33, &bytes);
    status += FT_Read(handle, &buffer, 33, &bytes);
    return 0 == status;
}

void WorkerThread::start(
    std::list<char*> *pages, quint16 pageNumber, bool isUpload
)
{
    emit disableButtons();
    emit progress(0);
    this->pages = pages;
    this->pageNumber = pageNumber;
    this->isUpload = isUpload;
    start();
}

void WorkerThread::run()
{
    clock_t timer = clock();

    FT_HANDLE handle;
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
            !strcmp(info.Manufacturer, "EKSELCOM") &&
            !strcmp(info.Description, "RS-SBI")
        ) {
            found = true;
            break;
        }

        FT_Close(handle);
    }

    if (!found)
    {
        emit status("ERROR: No device found");
        emit enableButtons();
        return;
    }

    if (
        FT_SetUSBParameters(handle, 65536, 0) != FT_OK ||
        FT_SetBitMode(handle, MOSI | CS, 0x4) != FT_OK ||
        FT_SetDivisor(handle, 0) != FT_OK ||
        FT_SetLatencyTimer(handle, 2) != FT_OK
    ) {
        FT_Close(handle);
        emit status("ERROR: Device initialization failed");
        emit enableButtons();
        return;
    }

    buffer[0] = CS;
    FT_Write(handle, &buffer, 1, &bytes);
    FT_Read(handle, &buffer, 1, &bytes);
    if (!isValid(handle)) {
        FT_Close(handle);
        emit status("ERROR: No flash connected");
        emit enableButtons();
        return;
    }

    emit status(isUpload ? "Uploading..." : "Verifying...");
    char *pageBuffer;
    bool firstBuffer = true, success = true;
    int pageId = 0;
    for (
        std::list<char*>::iterator i = pages->begin(); i != pages->end(); ++i
    ) {
        if (isUpload) {
            success = write(handle, firstBuffer, *i, pageId);
            firstBuffer = !firstBuffer;
        }
        else {
            pageBuffer = read(handle, pageId);
            for (int j = 0; j < 4224; ++j) {
                if (
                    (((*i)[j] > 0) && !(pageBuffer[j] & MISO)) ||
                    (!(*i)[j] && (pageBuffer[j] & MISO))
                ) {
                    success = false;
                    break;
                }
            }
            delete[] pageBuffer;
        }
        if (!success)
            break;

        emit progress((int)floor((++pageId * 100.0) / pageNumber));
    }

    FT_Close(handle);
    emit enableButtons();
    if (success)
        emit status(
            "Success! " + QString(isUpload ? "Uploaded" : "Verified") + " in " +
            QString::number((double)(clock() - timer) / CLOCKS_PER_SEC, 'f', 3)
            + "s"
        );
    else
        emit status(
            "ERROR: " + QString(isUpload ?
                "Upload failed, try again" :
                "Data is corrupt, reupload"
            )
        );
}
