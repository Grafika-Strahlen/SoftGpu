copy /b /y D:\GPU\SoftGpu\bin\x64\Release\VirtualBoxDevice.dll D:\GPU\SoftGpu\signing\VirtualBoxDevice.dll
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.22000.0\x86\Inf2Cat.exe" /driver:D:\GPU\SoftGpu\signing /os:10_VB_X64
"C:\Windows\System32\makecab.exe" /f VirtualBoxDevice.ddf
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.22000.0\x64\signtool.exe" sign /a /tr http://timestamp.globalsign.com/tsa/r6advanced1 /fd SHA256 /td SHA256 D:\GPU\SoftGpu\signing\disk1\VirtualBoxDevice.cab
pause
