#include <iostream>
#include <memory>
#include <windows.h>
#include "ftd2xx.h"

#include <QtCore/QDebug>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QSizePolicy>

#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

private slots:
    void pick();
    // void upload();
    // void verify();
    // void progress();

private:
    std::unique_ptr<QPushButton> fileButton;
    std::unique_ptr<QWidget> widget;
    std::unique_ptr<QGridLayout> grid;
    // QProgressBar *uploadBar;
    // QPushButton *uploadButton;
    // QPushButton *verifyButton;
};
