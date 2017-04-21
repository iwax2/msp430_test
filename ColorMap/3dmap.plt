set terminal png
set xl "x position"
set yl "y position"
set pm3d map interpolate 20,20
set palette rgbformula 22,13,-31
set output '01_temperature.png'
set cbrange[18:25]
set title "Temperature [C]"
splot "assmann_20170301.txt" using 1:2:3
set output '02_humidity.png'
set cbrange[45:80]
set title "Relative Humidity [%]"
splot "assmann_20170301.txt" using 1:2:5
set output '03_VPD.png'
set cbrange[0.6:1.7]
set title "Vapour Pressure Dificit [kPa]"
splot "assmann_20170301.txt" using 1:2:6
