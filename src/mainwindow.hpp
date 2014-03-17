#pragma once

#include <list>
#include <memory>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "common.hpp"
#include "workerthread.hpp"

using std::list;
using std::vector;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

public slots:
    void refresh();
    void upload();
    void verify();
    void status();
    void status(const QString &message);
    void progress(int percentage = 0);
    void buttons(Buttons state = Buttons::None);

private:
    bool load(const QString &path, quint16 offset);

    QWidget widget;
    QGridLayout grid;
    QPushButton refreshButton;
    QVBoxLayout fileListBox;
    QWidget fileListWidget;
    QScrollArea fileListArea;
    QVBoxLayout fileListAreaBox;
    QWidget fileListAreaWidget;
    QProgressBar progressBar;
    QHBoxLayout progressBarBox;
    QWidget progressBarWidget;
    QPushButton uploadButton;
    QPushButton verifyButton;

    vector<quint8> bytes;
    quint32 size;
    list<char *> pages;
    WorkerThread worker;
};
