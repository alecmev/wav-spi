from ctypes import *
import os
import sys

PVOID = c_void_p
BYTE = c_byte
CHAR = c_char
UCHAR = c_ubyte
INT = c_int
LONG = c_long
ULONG = c_ulong
WORD = c_ushort
DWORD = c_ulong
STRING = c_char_p
BUFFER = c_buffer

class FT_PROGRAM_DATA(Structure):
    _fields_ = [
        ('Signature1', DWORD),
        ('Signature2', DWORD),
        ('Version', DWORD),
        ('VendorId', WORD),
        ('ProductId', WORD),
        ('Manufacturer', STRING),
        ('ManufacturerId', STRING),
        ('Description', STRING),
        ('SerialNumber', STRING),
        ('MaxPower', WORD),
        ('PnP', WORD),
        ('SelfPowered', WORD),
        ('RemoteWakeup', WORD),
        ('Rev4', UCHAR),
        ('IsoIn', UCHAR),
        ('IsoOut', UCHAR),
        ('PullDownEnable', UCHAR),
        ('SerNumEnable', UCHAR),
        ('USBVersionEnable', UCHAR),
        ('USBVersion', WORD),
        ('Rev5', UCHAR),
        ('IsoInA', UCHAR),
        ('IsoInB', UCHAR),
        ('IsoOutA', UCHAR),
        ('IsoOutB', UCHAR),
        ('PullDownEnable5', UCHAR),
        ('SerNumEnable5', UCHAR),
        ('USBVersionEnable5', UCHAR),
        ('USBVersion5', WORD),
        ('AIsHighCurrent', UCHAR),
        ('BIsHighCurrent', UCHAR),
        ('IFAIsFifo', UCHAR),
        ('IFAIsFifoTar', UCHAR),
        ('IFAIsFastSer', UCHAR),
        ('AIsVCP', UCHAR),
        ('IFBIsFifo', UCHAR),
        ('IFBIsFifoTar', UCHAR),
        ('IFBIsFastSer', UCHAR),
        ('BIsVCP', UCHAR),
        ('UseExtOsc', UCHAR),
        ('HighDriveIOs', UCHAR),
        ('EndpointSize', UCHAR),
        ('PullDownEnableR', UCHAR),
        ('SerNumEnableR', UCHAR),
        ('InvertTXD', UCHAR),
        ('InvertRXD', UCHAR),
        ('InvertRTS', UCHAR),
        ('InvertCTS', UCHAR),
        ('InvertDTR', UCHAR),
        ('InvertDSR', UCHAR),
        ('InvertDCD', UCHAR),
        ('InvertRI', UCHAR),
        ('Cbus0', UCHAR),
        ('Cbus1', UCHAR),
        ('Cbus2', UCHAR),
        ('Cbus3', UCHAR),
        ('Cbus4', UCHAR),
        ('RIsVCP', UCHAR)
    ]

FT_OK = 0

if '_MEIPASS' in dir(sys):
    ftd2xx = WinDLL(os.path.join(sys._MEIPASS, 'ftd2xx.dll'))
else:
    ftd2xx = WinDLL('ftd2xx.dll')

FT_Open = ftd2xx.FT_Open # INT, POINTER(PVOID)
FT_Open.restype = ULONG

FT_Close = ftd2xx.FT_Close # PVOID
FT_Close.restype = ULONG

FT_EE_Read = ftd2xx.FT_EE_Read # PVOID, POINTER(FT_PROGRAM_DATA)
FT_EE_Read.restype = ULONG

FT_SetLatencyTimer = ftd2xx.FT_SetLatencyTimer # PVOID, UCHAR
FT_SetLatencyTimer.restype = ULONG

FT_SetUSBParameters = ftd2xx.FT_SetUSBParameters # PVOID, DWORD, DWORD
FT_SetUSBParameters.restype = ULONG

FT_SetBitMode = ftd2xx.FT_SetBitMode # PVOID, UCHAR, UCHAR
FT_SetBitMode.restype = ULONG

FT_SetBaudRate = ftd2xx.FT_SetBaudRate # PVOID, DWORD
FT_SetBaudRate.restype = ULONG

FT_SetDivisor = ftd2xx.FT_SetDivisor # PVOID, DWORD
FT_SetDivisor.restype = ULONG

FT_Read = ftd2xx.FT_Read # PVOID, POINTER(PVOID), DWORD, POINTER(DWORD)
FT_Read.restype = ULONG

FT_Write = ftd2xx.FT_Write # PVOID, POINTER(PVOID), DWORD, POINTER(DWORD)
FT_Write.restype = ULONG

FT_GetQueueStatus = ftd2xx.FT_GetQueueStatus # PVOID, POINTER(DWORD)
FT_GetQueueStatus.restype = ULONG

FT_Purge = ftd2xx.FT_Purge # PVOID, DWORD
FT_Purge.restype = ULONG
