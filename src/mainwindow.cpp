#include "mainwindow.hpp"

MainWindow::MainWindow()
{
    lastPath = QDir::currentPath();
    connect(&worker, SIGNAL(status(QString)), this, SLOT(status(QString)));
    connect(&worker, SIGNAL(progress(int)), this, SLOT(progress(int)));
    connect(&worker, SIGNAL(enableButtons()), this, SLOT(enableButtons()));
    connect(&worker, SIGNAL(disableButtons()), this, SLOT(disableButtons()));

    widget = std::unique_ptr<QWidget>(new QWidget(this));
    grid = std::unique_ptr<QGridLayout>(new QGridLayout);

    fileButton = std::unique_ptr<QPushButton>(
        new QPushButton("Pick a WAV file...")
    );
    connect(fileButton.get(), SIGNAL(clicked()), this, SLOT(pick()));
    grid->addWidget(fileButton.get(), 0, 0);

    progressBarWidget = std::unique_ptr<QWidget>(new QWidget);
    progressBarBox = std::unique_ptr<QBoxLayout>(
        new QBoxLayout(QBoxLayout::LeftToRight)
    );
    progressBar = std::unique_ptr<QProgressBar>(new QProgressBar);
    progress(0);
    progressBarBox->addWidget(progressBar.get(), 0);
    progressBarBox->setContentsMargins(1, 0, 0, 0);
    progressBarWidget->setLayout(progressBarBox.get());
    grid->addWidget(progressBarWidget.get(), 1, 0);

    uploadButton = std::unique_ptr<QPushButton>(new QPushButton("Upload"));
    uploadButton->setEnabled(false);
    connect(uploadButton.get(), SIGNAL(clicked()), this, SLOT(upload()));
    grid->addWidget(uploadButton.get(), 2, 0);

    verifyButton = std::unique_ptr<QPushButton>(new QPushButton("Verify"));
    verifyButton->setEnabled(false);
    connect(verifyButton.get(), SIGNAL(clicked()), this, SLOT(verify()));
    grid->addWidget(verifyButton.get(), 3, 0);

    widget->setLayout(grid.get());
    setCentralWidget(widget.get());
    setWindowTitle("RS-SBI");
    statusBar()->setSizeGripEnabled(false);
    update();
    setFixedSize(256, minimumSizeHint().height());
    setWindowFlags(Qt::MSWindowsFixedSizeDialogHint);
    show();

    
}

MainWindow::~MainWindow()
{
    clearPages();
}

void MainWindow::clearPages()
{
    pageNumber = 0;
    while (!pages.empty())
        delete pages.front(), pages.pop_front();
}

void MainWindow::pick()
{
    disableButtons();
    QString filePath = QFileDialog::getOpenFileName(
        this, "Pick WAV file", lastPath, "WAV (*.wav;*.wave;*.w64)"
    );
    QFile fileHandle(filePath);
    
    if (!fileHandle.exists()) {
        update();
        enableButtons(false);
        return;
    }

    QFileInfo fileInfo(filePath);
    lastPath = fileInfo.canonicalPath();

    if (!fileHandle.open(QIODevice::ReadOnly)) {
        update("ERROR: Failed to open the file");
        enableButtons(false);
        return;
    }
    
    char infoBuffer[4];

    fileHandle.seek(16);
    fileHandle.read(infoBuffer, 4);
    quint32 headerSize = qFromLittleEndian<quint32>((uchar*)infoBuffer);
    fileHandle.read(infoBuffer, 2);
    quint16 audioFormat = qFromLittleEndian<quint16>((uchar*)infoBuffer);
    fileHandle.read(infoBuffer, 2);
    quint16 channels = qFromLittleEndian<quint16>((uchar*)infoBuffer);
    fileHandle.read(infoBuffer, 4);
    quint32 sampleRate = qFromLittleEndian<quint32>((uchar*)infoBuffer);

    fileHandle.seek(34);
    fileHandle.read(infoBuffer, 2);
    quint16 bytesPerSample = qFromLittleEndian<quint16>((uchar*)infoBuffer) / 8;
    
    fileHandle.seek(headerSize + 24);
    fileHandle.read(infoBuffer, 4);
    quint32 dataSize = qFromLittleEndian<quint32>((uchar*)infoBuffer);
    quint32 samples = dataSize / 2;
    
    QString error = "";
    if (audioFormat != 1)
        error = "Not a valid WAV PCM file";
    else if (channels != 1)
        error = "Only mono supported";
    else if (bytesPerSample != 2)
        error = "Only 16 bits/sample supported";
    else if (dataSize > 2097144)
        error = "The file is larger than 2 MB";

    if (!error.isEmpty()) {
        fileHandle.close();
        update("ERROR: " + error);
        enableButtons(false);
        return;
    }

    char pageRawBuffer[528], *pageBuffer = NULL;
    quint8 pageOffset = 8;
    quint16 pageSize = 520, pageRawBufferSize, sample;
    quint32 remaining = dataSize;
    clearPages();
    while (remaining > 0) {
        pageRawBufferSize = remaining > pageSize ? pageSize : remaining;
        fileHandle.read(pageRawBuffer, pageRawBufferSize);
        pageBuffer = new char[4225]();
        pageBuffer[4224] = CS;
        for (int i = 0; i < pageRawBufferSize/2; ++i) {
            sample = qFromLittleEndian<quint16>(
                (uchar*)(pageRawBuffer + i*2)
            ) + 0x8000;
            Helper::encodeByte(
                pageBuffer, i*2 + pageOffset, (char)(sample & 0xFF)
            );
            Helper::encodeByte(
                pageBuffer, i*2 + pageOffset + 1, (char)(sample >> 8)
            );
        }

        remaining = remaining > pageSize ? remaining - pageSize : 0;
        if (pageSize == 520) {
            Helper::encodeByte(pageBuffer, 0, (char)(sampleRate & 0xFF));
            Helper::encodeByte(pageBuffer, 1, (char)((sampleRate >> 8) & 0xFF));
            Helper::encodeByte(
                pageBuffer, 2, (char)((sampleRate >> 16) & 0xFF)
            );
            Helper::encodeByte(pageBuffer, 3, (char)(sampleRate >> 24));
            Helper::encodeByte(pageBuffer, 4, (char)(samples & 0xFF));
            Helper::encodeByte(pageBuffer, 5, (char)((samples >> 8) & 0xFF));
            Helper::encodeByte(pageBuffer, 6, (char)((samples >> 16) & 0xFF));
            Helper::encodeByte(pageBuffer, 7, (char)(samples >> 24));

            pageRawBufferSize += 8;
            pageOffset = 0;
            pageSize = 528;
        }

        pages.push_back(pageBuffer);
        ++pageNumber;
    }

    fileHandle.close();
    update(
        "© 2013 Orekus Ltd.", fileInfo.completeBaseName(), dataSize, sampleRate
    );
    enableButtons();
}

void MainWindow::upload()
{
    worker.start(&pages, pageNumber, true);
}

void MainWindow::verify()
{
    worker.start(&pages, pageNumber, false);
}

void MainWindow::status(const QString &message)
{
    statusBar()->showMessage(message);
}

void MainWindow::progress(const int percentage)
{
    progressBar->setValue(percentage);
}

void MainWindow::enableButtons(bool all)
{
    fileButton->setEnabled(true);
    if (all) {
        uploadButton->setEnabled(true);
        verifyButton->setEnabled(true);
    }
}

void MainWindow::disableButtons(bool all)
{
    fileButton->setEnabled(false);
    if (all) {
        uploadButton->setEnabled(false);
        verifyButton->setEnabled(false);
    }
}

void MainWindow::update(const QString &message)
{
    status(message);
    progress(0);
    fileButton->setText("Pick a WAV file...");
    uploadButton->setEnabled(false);
    verifyButton->setEnabled(false);
}

void MainWindow::update(
    const QString &message, const QString &baseName,
    quint32 dataSize, quint32 sampleRate
)
{
    status(message);
    fileButton->setText(
        baseName + " [" + QString::number(
        (dataSize / (sampleRate * 2.0)), 'f', 3) + "s " +
        QString::number(sampleRate) + "Hz]"
    );
    enableButtons();
    progress(0);
}
