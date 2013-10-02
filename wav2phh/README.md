# Wav to pulse height histogram converter

Wav2phh is a command line based wav file to pulse height histogram converter. It may be used to
postprocess and generate a gamma spectrum from a scintillation counter record.


## Software and system requirements

Wav2phh can be build and run on linux host systems. Cross compilation on linux
for windows with mingw32-gcc is also possible. Cross compiled windows executables are confirmed
to work under Win7/64 and Win 32 bit versions.

  * [libsndfile API][libsndfile]
  * [gcc (Linux platform)][gcc]
  * [MinGW32 (Windows platform)][mingw]

[libsndfile]:  http://www.mega-nerd.com/libsndfile/
             "Libsndfile aplication programming interface"
[gcc]:       http://gcc.gnu.org/
             "GNU Compiler Collection"
[mingw]:     http://www.mingw.org
             " Minimalist GNU for Windows"

## Usage

Wav2phh is a command line program. On Windows open a command line window, seek to the program location
and type pulse_height_hist.exe. A helpscreen will show you all the possible options.

## Building

  * On linux systems install libsndfile, gcc and then type `make -f Makefile.Linux`
  * Cross compilation on linux for windows:
    * Build libsndfile-1.dll and libsndfile.a from libsndfile API with following switches:  
      ./configure --host=mingw32 --target=mingw32 --disable-alsa --disable-jack --disable-sqlite
    * Call either `make -f Makefile.Windows static` or `make -f Makefile.Windows dynamic`
      for getting statically or dynamically linked executables

I haven't tried for myself to build everything on a windows computer but there are people who made it.
My last information about this is: Install MinGW including the shell. Install also MSYS.
[Build libsndfile](http://svn.annodex.net/annodex-core/libsndfile-1.0.11/doc/win32.html)
then build wav2phh.

## Recommendations on sampling rate

Should be as high as possible.  
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


