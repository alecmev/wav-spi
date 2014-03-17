#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>
#include <windows.h>

#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QSize>
#include <QStatusBar>
#include <QString>
#include <QtEndian>
#include <QtMath>

#include "common.hpp"
#include "helper.hpp"

#include "mainwindow.hpp"

inline Buttons operator|(Buttons a, Buttons b) {
    return static_cast<Buttons>(
        static_cast<quint8>(a) | static_cast<quint8>(b)
    );
}

inline Buttons operator&(Buttons a, Buttons b) {
    return static_cast<Buttons>(
        static_cast<quint8>(a) & static_cast<quint8>(b)
    );
}

MainWindow::MainWindow() {
    int row = 0;
    qRegisterMetaType<Buttons>("Buttons");
    connect(&worker, SIGNAL(status(QString)), this, SLOT(status(QString)));
    connect(&worker, SIGNAL(progress(int)), this, SLOT(progress(int)));
    connect(&worker, SIGNAL(buttons(Buttons)), this, SLOT(buttons(Buttons)));

    refreshButton.setText("Refresh");
    grid.addWidget(&refreshButton, row++, 0);
    
    fileListBox.setSizeConstraint(QLayout::SetFixedSize);
    fileListWidget.setLayout(&fileListBox);
    fileListArea.setWidgetResizable(true);
    fileListArea.setWidget(&fileListWidget);
    fileListAreaBox.setContentsMargins(1, 0, 1, 3);
    fileListAreaBox.addWidget(&fileListArea);
    fileListAreaWidget.setFixedHeight(192);
    fileListAreaWidget.setLayout(&fileListAreaBox);
    grid.addWidget(&fileListAreaWidget, row++, 0);

    progressBar.setMinimum(0);
    progressBar.setMaximum(10000);
    progressBarBox.setContentsMargins(1, 0, 0, 0);
    progressBarBox.addWidget(&progressBar, 0);
    progressBarWidget.setLayout(&progressBarBox);
    grid.addWidget(&progressBarWidget, row++, 0);

    uploadButton.setText("Upload");
    grid.addWidget(&uploadButton, row++, 0);

    verifyButton.setText("Verify");
    grid.addWidget(&verifyButton, row++, 0);

    widget.setParent(this);
    widget.setLayout(&grid);
    setCentralWidget(&widget);

    setWindowTitle("wav-spi");
    statusBar()->setSizeGripEnabled(false);
    setFixedSize(384, minimumSizeHint().height());
    setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);

    connect(&refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    connect(&uploadButton, SIGNAL(clicked()), this, SLOT(upload()));
    connect(&verifyButton, SIGNAL(clicked()), this, SLOT(verify()));
    
    buttons(Buttons::Step1);
    progress();
    status();

    refresh();
    show();
}

void MainWindow::refresh() {
    buttons();
    for (QLayoutItem *item; (item = fileListBox.takeAt(0)) != NULL; ) {
        delete item->widget();
        delete item;
    }

    bytes.clear();
    bytes.shrink_to_fit();
    while (!pages.empty()) {
        delete pages.front();
        pages.pop_front();
    }

    QStringList files = QDir::current().entryList(
        QStringList() << "*.wav" << "*.wave", QDir::Filter::Files,
        QDir::SortFlag::Name | QDir::IgnoreCase
    );
    if (files.isEmpty()) {
        status("INFO: no WAV files found");
        buttons(Buttons::Step1);
        return;
    }

    quint32 count = files.size();
    size = count*4 + 4;
    bytes.resize(size);
    qToLittleEndian<quint32>(count, &bytes[0]);
    for (quint32 i = 0; i < count; ++i) {
        if (!load(files.at(i), i*4 + 4)) {
            buttons(Buttons::Step1);
            return;
        }
    }

    for (quint32 i = 0; i < size; i += PAGESIZE) {
        char *pageBuffer = new char[PAGEBITS + 1]();
        pages.push_back(pageBuffer);
        pageBuffer[PAGEBITS] = CS;
        quint32 bytesNeeded = (size - i) < PAGESIZE ? (size - i) : PAGESIZE;
        for (quint32 j = i, k = 0; j < i + bytesNeeded; ++j, ++k) {
            Helper::encodeByte(pageBuffer, k, bytes[i]);
        }
    }

    buttons(Buttons::Step2);
}

bool MainWindow::load(const QString &path, const quint16 offset) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        status("ERROR: failed to open " + path);
        return false;
    }
    
    quint8 buffer[PAGESIZE];

    file.seek(16);
    file.read((char *)buffer, 4);
    quint32 headerSize = qFromLittleEndian<quint32>(buffer);
    file.read((char *)buffer, 2);
    quint16 audioFormat = qFromLittleEndian<quint16>(buffer);
    file.read((char *)buffer, 2);
    quint16 channels = qFromLittleEndian<quint16>(buffer);
    file.read((char *)buffer, 4);
    quint32 sampleRate = qFromLittleEndian<quint32>(buffer);

    file.seek(34);
    file.read((char *)buffer, 2);
    quint16 bytesPerSample = qFromLittleEndian<quint16>(buffer) / 8;
    
    file.seek(headerSize + 24);
    file.read((char *)buffer, 4);
    quint32 dataSize = qFromLittleEndian<quint32>(buffer);
    quint32 samples = dataSize / 2;
    
    QString error = "";
    if (audioFormat != 1) {
        error = "only LPCM supported";
    }
    else if (channels != 1) {
        error = "only mono supported";
    }
    else if (bytesPerSample != 2) {
        error = "only 16 bits per sample supported";
    }

    if (!error.isEmpty()) {
        status("ERROR: " + error);
        file.close();
        return false;
    }

    bytes.resize(size + dataSize + 8);
    qToLittleEndian<quint32>(size, &bytes[offset]);
    qToLittleEndian<quint32>(sampleRate, &bytes[size]);
    qToLittleEndian<quint32>(samples, &bytes[size + 4]);
    size += 8;

    quint32 remaining = dataSize;
    while (remaining) {
        quint16 readcount = remaining > PAGESIZE ? PAGESIZE : remaining;
        remaining -= readcount;
        quint32 i = size + 1;
        if (file.read((char *)&bytes[size], readcount) != readcount) {
            status("ERROR: failed to read " + path);
            file.close();
            return false;
        }

        size += readcount;
        for (; i < size; i += 2) {
            bytes[i] += 0x80;
        }
    }

    fileListBox.addWidget(new QLabel(
        path + " [" + 
            QString::number((dataSize / (sampleRate * 2.0)), 'f', 3) + "s " +
            QString::number(sampleRate) + "Hz " +
            QString::number(qCeil(dataSize / 1024.0)) + " KiB"
        "]"
    ));
    file.close();
    return true;
}

void MainWindow::upload() {
    buttons();
    worker.start(pages, true);
}

void MainWindow::verify() {
    buttons();
    worker.start(pages, false);
}

void MainWindow::status() {
    status("© 2014 Olegs Jeremejevs");
}

void MainWindow::status(const QString &message) {
    statusBar()->showMessage(message);
}

void MainWindow::progress(const int percentage) {
    progressBar.setValue(percentage);
}

void MainWindow::buttons(const Buttons state) {
    refreshButton.setEnabled((quint8)(state & Buttons::Refresh));
    uploadButton.setEnabled((quint8)(state & Buttons::Upload));
    verifyButton.setEnabled((quint8)(state & Buttons::Verify));
}
