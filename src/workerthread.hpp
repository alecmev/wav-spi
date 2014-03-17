#pragma once

#include <list>
#include <windows.h>

#include <QString>
#include <QThread>
#include <QtGlobal>

#include "ftd2xx.h"

#include "common.hpp"

class WorkerThread : public QThread {
    Q_OBJECT
    using QThread::start;

public slots:
    void start(std::list<char *> &pages, bool isUpload);

signals:
    void status(const QString &message);
    void progress(int percentage);
    void buttons(Buttons state);

protected:
    void run() override;

private:
    quint32 pagesPerSector(quint32 flashsize);
    void encodeHeader(char instruction, quint16 pageId);
    char getStatus();
    void whileBusy();
    void whileReadOnly();
    bool writeByte(char byte);
    bool write(char *pageBuffer, quint16 pageId);
    char *read(quint16 pageId);

    FT_HANDLE handle;
    char buffer[PAGEBITS + 33];
    DWORD bytes;
    std::list<char *> pages;
    quint16 pageNumber;
    bool isUpload;
};
