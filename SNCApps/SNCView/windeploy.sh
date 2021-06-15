c:/Qt/5.12.6/msvc2017_64/bin/qtenv2.bat
cd c:/SNC2/SNCApps/SNCView
qmake
nmake
cd release
mkdir SNCView
copy SNCView.exe SNCView
cd SNCView
windeployqt .

