# Wav to pulse height histogram converter

Wav2phh is a Qt frontend based audio/wav to pulse height histogram converter. It may be used to postprocess and generate a gamma spectrum from a scintillation counter wav record.

[Screenshot](https://github.com/samplemaker/wav2phh/tree/wav2phh_github/screenshots/wav2phh_in_action.png)

## Software and system requirements

Wav2phh can be build for linux and windows. Prerequisites:

  * [Qt API (Version 5)][QtHomepage]
  * [g++ - Gnu Compiler Collection (Linux platform)][gcc]
  * [MinGW32 (Windows platform)][mingw]

[QtHomepage]:  http://qt-project.org/
             "Qt aplication programming interface"
[gcc]:       http://gcc.gnu.org/
             "GNU Compiler Collection"
[mingw]:     http://www.mingw.org/
             "Minimalist GNU for Windows"  

## Building

  * On linux systems with Qt5: Execute `/usr/bin/qmake-qt5 wav2phh.pro` and then type `make`
  * Cross compilation on linux for 32bit executables for windows:
    * Create a new profile with `/usr/bin/i686-w64-mingw32-qmake-qt5 -project`. Add to your profile:  
      QT += widgets  
      QT += multimedia
    * Create the makefile with `/usr/bin/i686-w64-mingw32-qmake-qt5`
    * Compile and link with `make`
  * On Windows: ???
    * Install QT [Install QT for Windows](http://qt-project.org/doc/qt-5/windows-building.html)
    * To compile the program proceed as on linux systems


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


