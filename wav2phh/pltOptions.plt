#  pltOptions.plt
#
#  Gnuplot options
#  Copyright (C) 2010 samplemaker
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation; either version 2.1
#  of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free
#  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA 02110-1301 USA

set title "Histogram" font "Arial,16"
#set logscale y
#set nologscale y
#set yrange [0:700]
set ylabel 'Counts'
set grid ytics lt 0 lw 1 lc rgb "#909070"
#set xrange [0:15]
set xlabel 'Channel No.'
set grid xtics lt 0 lw 1 lc rgb "#909070"

#####################

#If you want to use the markers you have set xrange = x2range

#set xrange [0:300]
#set x2range [0:300]

##### reference
refchan = 61
refenergy = 239
refenergystr = sprintf("REFERENCE: %1.1f keV",refenergy)

#### markers
chan1 = 47
energy1 = refenergy*chan1/refchan;
energystr1 = sprintf("MARKER1: %1.1f keV",energy1)

chan2 = 25
energy2 = refenergy*chan2/refchan;
energystr2 = sprintf("MARKER2: %1.1f keV",energy2)

chan3 = 17
energy3 = refenergy*chan3/refchan;
energystr3 = sprintf("MARKER3: %1.1f keV",energy3)

set x2tics (energystr1 chan1, energystr2 chan2, energystr3 chan3, refenergystr refchan)

# set x2tics ("XYZ keV" 100,"ABC keV" 200)

#####
# set datafile separator ";"
set x2tics rotate 
set grid x2tics lt 0 lw 1 lc rgb "#909070"
set x2label "Energy [keV]"
set size ratio 0.45
#set multiplot
set nomultiplot
set key top right
set key box
set style fill solid 1.0
#set terminal postscript eps enhanced color solid;
#set output "export.eps"
#plot "hist1.dat" lt 3 with lines title "hits1" , \
#"hist2.dat" lt 2 with lines title "hist2" , \
#"hist3.dat" using 2 with histograms title "xyz"
