#ifndef MAIN_WINDOW
#define MAIN_WINDOW

#include <iostream>
#include <memory>
#include <windows.h>
#include <string>
#include <unistd.h>
#include "ftd2xx.h"
#include "helper.hpp"
#include "workerthread.hpp"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/qendian.h>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QIODevice>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QString>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizeGrip>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

public slots:
    void pick();
    void upload();
    void verify();

    void status(const QString &message);
    void progress(int percentage);
    void enableButtons(bool all = true);
    void disableButtons(bool all = true);

private:
    void update(const QString &status = "© 2013 Orekus Ltd.");
    void update(
        const QString &status, const QString &baseName,
        quint32 dataSize, quint32 sampleRate
    );

    std::unique_ptr<QWidget> widget;
    std::unique_ptr<QGridLayout> grid;
    std::unique_ptr<QPushButton> fileButton;
    std::unique_ptr<QWidget> progressBarWidget;
    std::unique_ptr<QBoxLayout> progressBarBox;
    std::unique_ptr<QProgressBar> progressBar;
    std::unique_ptr<QPushButton> uploadButton;
    std::unique_ptr<QPushButton> verifyButton;

    void clearPages();

    std::list<char*> pages;
    quint16 pageNumber;
    WorkerThread worker;
    QString lastPath;
};

#endif
