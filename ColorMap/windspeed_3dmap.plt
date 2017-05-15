set terminal png
set xl "x position"
set yl "y position"
set pm3d map interpolate 20,20
set palette rgbformula 22,13,-31
set output '01_temperature.png'
set cbrange[26:32]
set title "Temperature [C]"
splot "windspeed_20170502.txt" using 1:2:3
set output '04_windspeed.png'
set cbrange[0:0.6]
set title "Wind speed [m/s]"
splot "windspeed_20170502.txt" using 1:2:4
