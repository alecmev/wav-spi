#include "mainwindow.h"

MainWindow::MainWindow()
{
    widget = std::unique_ptr<QWidget>(new QWidget(this));
    grid = std::unique_ptr<QGridLayout>(new QGridLayout);

    fileButton = std::unique_ptr<QPushButton>(
        new QPushButton("Pick an audio file...")
    );
    connect(fileButton.get(), SIGNAL(clicked()), this, SLOT(pick()));
    grid->addWidget(fileButton.get());

    widget->setLayout(grid.get());
    setCentralWidget(widget.get());
    setWindowTitle("RS-SBI");
    
    statusBar()->showMessage("Ready");
    statusBar()->setSizeGripEnabled(false);
    adjustSize();
    setFixedSize(256, height());
    show();
}

MainWindow::~MainWindow()
{
    fileButton.release();
    grid.release();
    widget.release();
}

void MainWindow::pick()
{
    std::cout << "pick";

//     QFile file(fileName);
//     if (!file.open(QFile::ReadOnly | QFile::Text)) {
//         QMessageBox::warning(this, tr("Application"),
//                              tr("Cannot read file %1:\n%2.")
//                              .arg(fileName)
//                              .arg(file.errorString()));
//         return;
//     }

//     QTextStream in(&file);
// #ifndef QT_NO_CURSOR
//     QApplication::setOverrideCursor(Qt::WaitCursor);
// #endif
//     textEdit->setPlainText(in.readAll());
// #ifndef QT_NO_CURSOR
//     QApplication::restoreOverrideCursor();
// #endif

//     setCurrentFile(fileName);
//     statusBar()->showMessage(tr("File loaded"), 2000);
}
