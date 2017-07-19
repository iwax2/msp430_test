set dgrid3d splines 80,120
set pm3d map
set palette defined ( 0 '#000090',\
                      1 '#000fff',\
                      2 '#0090ff',\
                      3 '#0fffee',\
                      4 '#90ff70',\
                      5 '#ffee00',\
                      6 '#ff7000',\
                      7 '#ee0000',\
                      8 '#7f0000')
set term png
set output 'demo/colormap_vpd.png'
set cbrange[1:3]
set title strftime("VPD[kPa] colormap plotted at %Y/%m/%d %H:%M:%S", time(0)+9*3600)
splot "../matrix_vpd.txt" notitle with pm3d



