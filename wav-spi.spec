# -*- mode: python -*-
a = Analysis(['wav-spi.py'],
             pathex=['C:\\STORAGE\\stuff\\work\\ekselcom\\2013.05.26.wav-spi'],
             hiddenimports=[],
             hookspath=None)
pyz = PYZ(a.pure)
exe = EXE(pyz,
          a.scripts,
          a.binaries + [('ftd2xx.dll', 'ftd2xx.dll', 'BINARY')],
          a.zipfiles,
          a.datas,
          name=os.path.join('dist', 'wav-spi.exe'),
          debug=False,
          strip=None,
          upx=True,
          console=False )
app = BUNDLE(exe,
             name=os.path.join('dist', 'wav-spi.exe.app'))
