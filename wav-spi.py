from math import floor
import os
import struct
import sys
from time import clock, sleep
import wave

from PyQt4 import QtGui, QtCore

from ftd2xx import *

CLK  = 0x01 # TxD >
MOSI = 0x02 # RxD >
MISO = 0x20 # DSR <
CS   = 0x10 # DTR >

CONTINUOUS_ARRAY_READ           = 0xE8
MAIN_PAGE_READ                  = 0xD2
BUFFER1_READ                    = 0xD4
BUFFER2_READ                    = 0xD6
STATUS_READ                     = 0xD7
DEVICE_ID_READ                  = 0x9F

BUFFER1_WRITE                   = 0x84
BUFFER2_WRITE                   = 0x87
BUFFER1_TO_MAIN_WRITE_ERASE     = 0x83
BUFFER2_TO_MAIN_WRITE_ERASE     = 0x86
BUFFER1_TO_MAIN_WRITE           = 0x88
BUFFER2_TO_MAIN_WRITE           = 0x89
PAGE_ERASE                      = 0x81
BLOCK_ERASE                     = 0x50
MAIN_PAGE_PROGRAM_USING_BUFFER1 = 0x82
MAIN_PAGE_PROGRAM_USING_BUFFER2 = 0x85

MAIN_TO_BUFFER1_READ            = 0x53
MAIN_TO_BUFFER2_READ            = 0x55
MAIN_TO_BUFFER1_COMPARE         = 0x60
MAIN_TO_BUFFER2_COMPARE         = 0x61
AUTO_PAGE_REWRITE_TROW_BUFFER1  = 0x58
AUTO_PAGE_REWRITE_TROW_BUFFER2  = 0x59
SECURITY_WRITE                  = 0x9A

class Status():

    def __init__(self, byte):
        self.ready = bool(byte & 0x80)
        self.different = bool(byte & 0x40)
        self.valid = (byte & 0x3C) == 0x2C
        self.protected = bool(byte & 0x02)
        self.pageSize = bool(byte & 0x01)

    def __str__(self):
        return (
            'ready:     ' + str(self.ready) + '\n' +
            'different: ' + str(self.different) + '\n' +
            'valid:     ' + str(self.valid) + '\n' +
            'protected: ' + str(self.protected) + '\n' +
            'pageSize:  ' + str(512 if self.pageSize else 528)
        )

def encodeByte(byte):
    result = []
    while byte > 0:
        bit = MOSI * (byte % 2)
        byte >>= 1
        result.insert(0, bit)
        result.insert(1, bit | CLK)

    return [0, CLK] * (8 - len(result) / 2) + result

def decodeByte(byte):
    ignore = False
    result = 0
    power = 128
    for bit in byte:
        ignore = not ignore
        if ignore:
            continue

        result += power * ((bit & MISO) / MISO)
        power >>= 1

    return result

def encodeData(data):
    result = [0]
    for byte in data:
        result += encodeByte(byte)

    return result + [0]

def decodeData(data):
    dataLen = len(data)
    data = data[1:dataLen - 1]
    result = []
    for i in range(dataLen / 16):
        result.append(decodeByte(data[i*16:i*16 + 16]))

    return result

def disable(handle):
    FT_Write(handle, byref(BYTE(CS)), DWORD(1), byref(DWORD()))
    FT_Read(handle, byref(BYTE()), DWORD(1), byref(DWORD()))

def getStatus(handle):
    data = encodeData([STATUS_READ, STATUS_READ])
    dataLen = len(data)
    bytes = DWORD()
    FT_Write(
        handle, byref((BYTE * dataLen)(*data)), DWORD(dataLen), byref(bytes)
    )
    disable(handle)
    status = (BYTE * dataLen)()
    FT_Read(handle, byref(status), DWORD(dataLen), byref(bytes))
    return Status(decodeByte(status[dataLen - 17:dataLen - 1]))

def read(handle, blockId):
    data = encodeData(
        [MAIN_PAGE_READ, (blockId & 0xFC0) >> 6, (blockId & 0x3F) << 2, 0] +
        [0] * 532
    )
    dataLen = len(data)
    bytes = DWORD()
    FT_Write(
        handle, byref((BYTE * dataLen)(*data)), DWORD(dataLen), byref(bytes)
    )
    disable(handle)
    block = (BYTE * dataLen)()
    FT_Read(
        handle, byref(block), DWORD(dataLen), byref(bytes)
    )
    return decodeData(block)

def write(handle, block, blockLen, blockId, BUFFER_W, BUFFER_TO_MAIN_WE):
    data = encodeData([BUFFER_W, 0, 0, 0] + block)
    dataLen = len(data)
    bytes = DWORD()
    FT_Write(
        handle, byref((BYTE * dataLen)(*data)), DWORD(dataLen), byref(bytes)
    )
    disable(handle)
    FT_Read(
        handle, byref((BYTE * dataLen)()), DWORD(dataLen), byref(bytes)
    )
    while not getStatus(handle).ready:
        sleep(0.0001)
        pass

    data = encodeData([
        BUFFER_TO_MAIN_WE, (blockId & 0xFC0) >> 6, (blockId & 0x3F) << 2, 0
    ])
    dataLen = len(data)
    FT_Write(
        handle, byref((BYTE * dataLen)(*data)), DWORD(dataLen), byref(bytes)
    )
    disable(handle)
    FT_Read(
        handle, byref((BYTE * dataLen)()), DWORD(dataLen), byref(bytes)
    )

def write1(handle, block, blockLen, blockId):
    write(
        handle, block, blockLen, blockId, 
        BUFFER1_WRITE, BUFFER1_TO_MAIN_WRITE_ERASE
    )

def write2(handle, block, blockLen, blockId):
    write(
        handle, block, blockLen, blockId, 
        BUFFER2_WRITE, BUFFER2_TO_MAIN_WRITE_ERASE
    )

class MainWindow(QtGui.QMainWindow):

    def __init__(self):
        super(MainWindow, self).__init__()

        widget = QtGui.QWidget()
        grid = QtGui.QGridLayout()
        widget.setLayout(grid)

        self.fileButton = QtGui.QPushButton('Pick an audio file...')
        self.fileButton.clicked.connect(self.pick)

        self.uploadBar = QtGui.QProgressBar()
        self.progress = self.uploadBar.setValue
        self.progress(0)

        self.uploadButton = QtGui.QPushButton('Upload')
        self.uploadButton.clicked.connect(self.upload)
        self.uploadButton.setEnabled(False)
        
        grid.addWidget(self.fileButton, 0, 0)
        grid.addWidget(self.uploadBar, 1, 0)
        grid.addWidget(self.uploadButton, 2, 0)

        self.status = self.statusBar().showMessage
        self.status('Ready')
        self.worker = WorkerThread(
            self.status, self.progress, self.enableButtons
        )
        self.statusBar().setSizeGripEnabled(False)
        self.setCentralWidget(widget)
        self.setWindowTitle('RS-SBI')
        self.adjustSize()
        self.setFixedSize(256, self.height())
        self.show()

    def pick(self):
        self.filePath = str(QtGui.QFileDialog.getOpenFileName(
            self, 'Pick WAV file', os.getcwd(), 'WAV (*.wav;*.wave;*.w64)'
        ))

        if os.path.exists(self.filePath):
            self.fileHandle = open(self.filePath, 'rb')
            self.fileHandle.seek(16)
            self.headerSize = struct.unpack('<I', self.fileHandle.read(4))[0]
            self.audioFormat = struct.unpack('<H', self.fileHandle.read(2))[0]
            self.channels = struct.unpack('<H', self.fileHandle.read(2))[0]
            self.sampleRate = struct.unpack('<I', self.fileHandle.read(4))[0]
            self.fileHandle.seek(6, 1)
            self.bytesPerSample = struct.unpack(
                '<H', self.fileHandle.read(2)
            )[0] / 8
            self.fileHandle.seek(self.headerSize - 12, 1)
            self.dataSize = struct.unpack('<I', self.fileHandle.read(4))[0]
            self.fileHandle.close()

            if self.audioFormat != 1:
                self.update(False, 'ERROR: Not a valid WAV PCM file')
                return

            if self.channels != 1:
                self.update(False, 'ERROR: Only mono supported')
                return
            
            if self.bytesPerSample != 2:
                self.update(False, 'ERROR: Only 16 bits/sample supported')
                return

            if self.dataSize > 2097144:
                self.update(False, 'ERROR: The file is larger than 2 MB')
                return

            self.update(True)
        else:
            self.update(False)

    def update(self, ready, status='Ready'):
        self.status(status)
        self.fileButton.setText((
            os.path.basename(self.filePath) + (' [%.3f' %
            (self.dataSize / (self.sampleRate * 2.0))) + 's ' +
            str(self.sampleRate) + 'Hz]'
            ) if ready else 'Pick WAV file...'
        )
        self.uploadButton.setEnabled(ready)
        self.progress(0)

    def upload(self):
        self.disableButtons()
        if not os.path.exists(self.filePath):
            self.update(False, 'ERROR: File does not exist')
            self.enableButtons()
            return

        handle = PVOID()
        info = FT_PROGRAM_DATA(
            Signature1=0, Signature2=0xffffffff, Version=2,
            Manufacturer = cast(BUFFER(256), STRING),
            ManufacturerId = cast(BUFFER(256), STRING),
            Description = cast(BUFFER(256), STRING),
            SerialNumber = cast(BUFFER(256), STRING)
        )
        found = False
        i = -1
        while True:
            i += 1
            if FT_Open(i, byref(handle)) != FT_OK:
                FT_Close(handle)
                break

            if (
                FT_EE_Read(handle, byref(info)) == FT_OK and
                info.Manufacturer == 'EKSELCOM' and
                info.Description == 'RS-SBI'
            ):
                found = True
                break

            FT_Close(handle)

        if not found:
            self.status('ERROR: No device found')
            self.enableButtons()
            return

        if (
            FT_SetUSBParameters(handle, DWORD(65536), DWORD(0)) != FT_OK or
            FT_SetBitMode(handle, UCHAR(0), UCHAR(0)) != FT_OK or
            FT_SetBitMode(
                handle, UCHAR(CLK | MOSI | CS), UCHAR(0x4)
            ) != FT_OK or
            FT_SetDivisor(handle, DWORD(0)) != FT_OK or
            FT_SetLatencyTimer(handle, UCHAR(2)) != FT_OK
        ):
            FT_Close(handle)
            self.status('ERROR: Device initialization failed')
            self.enableButtons()
            return

        disable(handle)
        if not getStatus(handle).valid:
            FT_Close(handle)
            self.status('ERROR: No flash connected')
            self.enableButtons()
            return

        self.status('Uploading...')
        self.worker.start(
            handle, self.filePath, self.sampleRate,
            self.headerSize, self.dataSize
        )

    def disableButtons(self):
        self.fileButton.setEnabled(False)
        self.uploadButton.setEnabled(False)

    def enableButtons(self):
        self.fileButton.setEnabled(True)
        self.uploadButton.setEnabled(True)

class WorkerThread(QtCore.QThread):

    status = QtCore.pyqtSignal('QString')
    progress = QtCore.pyqtSignal(int)
    enableButtons = QtCore.pyqtSignal()

    def __init__(self, status, progress, enableButtons):
        QtCore.QThread.__init__(self)
        self.status.connect(status)
        self.progress.connect(progress)
        self.enableButtons.connect(enableButtons)

    def start(self, handle, filePath, sampleRate, headerSize, dataSize):
        self.handle = handle
        self.filePath = filePath
        self.sampleRate = sampleRate
        self.headerSize = headerSize
        self.dataSize = dataSize
        self.samples = dataSize / 2
        QtCore.QThread.start(self)

    def run(self):
        lastClock = clock()
        fileHandle = open(self.filePath, 'rb')
        fileHandle.seek(self.headerSize + 28)
        try:
            pageSize = 520
            remaining = self.dataSize
            useFirst = True
            pageId = 0
            while remaining > 0:
                pageRaw = fileHandle.read(
                    pageSize if remaining >= pageSize else remaining
                )
                pageLen = len(pageRaw)
                page = []
                for i in range(pageLen / 2):
                    sample = struct.unpack(
                        '<h', pageRaw[i*2:i*2 + 2]
                    )[0] + 0x8000
                    page.append(sample & 0xFF)
                    page.append(sample >> 8)

                remaining -= pageSize
                if remaining < 0:
                    remaining = 0

                if pageSize == 520:
                    page = [
                        self.sampleRate & 0xFF,
                        (self.sampleRate >> 8) & 0xFF,
                        (self.sampleRate >> 16) & 0xFF,
                        self.sampleRate >> 24,
                        self.samples & 0xFF,
                        (self.samples >> 8) & 0xFF,
                        (self.samples >> 16) & 0xFF,
                        self.samples >> 24,
                    ] + page
                    pageLen += 8
                    pageSize = 528

                if useFirst:
                    write1(self.handle, page, pageLen, pageId)
                else:
                    write2(self.handle, page, pageLen, pageId)

                self.progress.emit(
                    100 - int(floor((remaining * 100.0) / self.dataSize))
                )
                useFirst = not useFirst
                pageId += 1

            self.status.emit('Success! Loaded in %.3fs' % (clock() - lastClock))
        except Exception as e:
            print e
            self.status.emit('ERROR: Upload failed, try again')
            
        FT_Close(self.handle)
        fileHandle.close()
        self.enableButtons.emit()

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    ex = MainWindow()
    sys.exit(app.exec_())