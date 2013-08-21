#ifndef WORKER_THREAD
#define WORKER_THREAD

#include <windows.h>
#include <algorithm>
#include <time.h>
#include "ftd2xx.h"
#include "helper.hpp"

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>

#define CS              0x10 // DTR >

#define MAIN_PAGE_READ  0xD2
#define STATUS_READ     0xD7
#define BUFFER1_WRITE   0x84
#define BUFFER2_WRITE   0x87
#define BUFFER1_FLUSH   0x83
#define BUFFER2_FLUSH   0x86

class WorkerThread : public QThread
{
    Q_OBJECT
    using QThread::start;

public slots:
    void start(
        std::list<char*> *pages, quint16 pageNumber, bool isUpload
    );

signals:
    void status(QString message);
    void progress(int percentage);
    void enableButtons();
    void disableButtons();

protected:
    void run() override;

private:
    void disable(FT_HANDLE handle);
    char getStatus(FT_HANDLE handle);
    bool isValid(FT_HANDLE handle);
    bool isReady(FT_HANDLE handle);
    char* read(FT_HANDLE handle, quint16 pageId);
    bool write(
        FT_HANDLE handle, bool firstBuffer, char *pageBuffer, quint16 pageId
    );

    char buffer[4289]; // (528 + 8) * 8 + 1
    DWORD bytes;
    std::list<char*> *pages;
    quint16 pageNumber;
    bool isUpload;
};

#endif
