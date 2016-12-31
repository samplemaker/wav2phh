# Wav to pulse height histogram converter

Wav2phh is a Qt frontend based audio/wav to pulse height histogram converter. It may be used to postprocess and generate a gamma spectrum from a scintillation counter wav record.

[Screenshot](https://github.com/samplemaker/wav2phh/tree/wav2phh_github/screenshots/wav2phh_in_action.png)

## Software and system requirements

Wav2phh can be build for linux and windows. Prerequisites:

  * [Qt API (Version 5)][QtHomepage]
  * [g++ - Gnu Compiler Collection (Linux platform)][gcc]

[QtHomepage]:  https://www.qt.io/developers/
               "Qt aplication programming interface"
[gcc]:       http://gcc.gnu.org/
             "GNU Compiler Collection"


## Building

  * On Linux:
    * Install the packages qt5-qtbase-devel and qt5-qtmultimedia-devel 
    * Execute `/usr/bin/qmake-qt5 wav2phh.pro`
    * Type `make`
  * On Windows:
    * Install Qt 5.6.2 for Windows 32-bit (MinGW 4.9.2, 1.0 GB) [qt-opensource-windows-x86-mingw492-5.6.2.exe](https://www.qt.io/download-open-source/)
    * Building from within qtcreator:  
      open `wav2phh.pro`  
      press the "play button"
    * Building from cmd line:  
      Set the environment `set PATH=C:\Qt\Qt5.6.2\5.6\mingw49_32\bin;C:\Qt\Qt5.6.2\Tools\mingw492_32\bin;C:\Qt\Qt5.6.2\Tools\QtCreator\bin;C:32;C:`  
      Type `qmake.exe -spec win32-g++ "CONFIG+=debug" "CONFIG+=qml_debug" -o Makefile wav2phh.pro`  
      Type `mingw32-make -f Makefile.Debug`


## Recommendations on sampling rate

Depending on the shaper output a very rough estimation can be made as follows:
Assume an input pulse shape u(t)~exp(-(a<font face="Symbol">&#42;</font>t)^2). It's fourier transform equals to 
U(<font face="Symbol">w</font>)~exp(-(<font face="Symbol">w</font>/2a)^2).
U is mainly located within <font face="Symbol">w</font>=-6a..6a (+-3 standard deviations) 
therefore U has a bandwith of approximately B=6a. To avoid aliasing during sampling 
the Nyquist Shannon theorem recommends a sampling frequency <font face="Symbol">w</font>s>2B.
With Ts=2pi/<font face="Symbol">w</font>s one will get Ts=1/2a and therefore the sampling points
at the positions n<font face="Symbol">&#42;</font>Ts are given as u(n<font face="Symbol">&#42;</font>Ts)=exp(-(n/2)^2).
u is approximately located within n=-6 .. 6 and therefore
minimum 12 sampling points per pulse should be considered for sampling rate 
calculations. Lower sampling rates are possible if the aliasing error made is allowable.


## Notes on shaping hardware

I have obtained good results using a
[variation of the semi-gaussian filter](https://github.com/samplemaker/wav_mca_demonstrator/blob/public/wav2phh/wav2phh.pdf)
of second order.

## The License

LGPLv2.1+


