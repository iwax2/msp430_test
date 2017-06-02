#!/usr/bin/python
# -*- coding: utf-8 -*-

# カラーマップを表示する

import matplotlib
matplotlib.rcParams['text.latex.unicode'] = True
matplotlib.use("Agg") # サーバサイドでXを不要にする
import matplotlib.pyplot as plt # 描画用
import numpy as np # 数値計算
from scipy import interpolate # スプライン補間
from scipy.interpolate import Rbf # RBF補間
import matplotlib.cm as cm # カラーバーの設定

# 参考 http://www.mwsoft.jp/programming/numpy/csv.html
data = np.loadtxt("html_20170529.txt", delimiter="\t", dtype=np.float64, usecols=(0,1,3)) # , filling_values=(0, 0, 0) 
#data = np.loadtxt("assmann_20170302.txt", delimiter="\t", dtype=np.float64, usecols=(0,1,2)) # , filling_values=(0, 0, 0) 
#data = np.loadtxt("windspeed_test.txt", delimiter="\t", dtype=np.float64, usecols=(0,1,2)) # , filling_values=(0, 0, 0) 

# 参考 http://www.mwsoft.jp/programming/numpy/csv.html
x = data[:,0]
y = data[:,1]
z = data[:,2]


'''
xa = []
ya = []
za = []

xa.append(1)
ya.append(3)
za.append((z[0]+z[1])/2)
xa.append(1)
ya.append(7)
za.append((z[1]+z[2])/2)

xa.append(6)
ya.append(3)
za.append((z[3]+z[4])/2)
xa.append(6)
ya.append(7)
za.append((z[4]+z[5])/2)

xa.append(8)
ya.append(3)
za.append((z[6]+z[7])/2)
xa.append(8)
ya.append(7)
za.append((z[7]+z[8])/2)

xa.append(10)
ya.append(3)
za.append((z[9]+z[10])/2)
xa.append(10)
ya.append(7)
za.append((z[10]+z[11])/2)

xa.append(15)
ya.append(3)
za.append((z[12]+z[13])/2)
xa.append(15)
ya.append(7)
za.append((z[13]+z[14])/2)

xa.append(3)
ya.append(1)
za.append((z[0]+z[3])/2)
xa.append(3)
ya.append(3)
za.append((za[0]+za[2])/2)
xa.append(3)
ya.append(5)
za.append((z[1]+z[4])/2)
xa.append(3)
ya.append(7)
za.append((za[1]+za[3])/2)
xa.append(3)
ya.append(10)
za.append((z[2]+z[5])/2)

xa.append(13)
ya.append(2)
za.append((z[9]+z[12])/2)
xa.append(13)
ya.append(3)
za.append((za[6]+za[8])/2)
xa.append(13)
ya.append(5)
za.append((z[10]+z[13])/2)
xa.append(13)
ya.append(7)
za.append((za[7]+za[9])/2)
xa.append(13)
ya.append(10)
za.append((z[11]+z[14])/2)

x = np.append(x,xa)
y = np.append(y,ya)
z = np.append(z,za)

print(x)
'''
#x,y = np.meshgrid(np.arange(0, 10, 0.1), np.arange(0, 10, 0.1))
#z = 2*np.random.randn(100,100)+20

#x=np.array([[1,1,1],[6,6,6],[8,8,8],[10,10,10],[15,15,15]])
#y=np.array([[1,5,10],[1,5,10],[1,5,10],[1,5,10],[1,5,10]])
#z=np.array([[20.0,20.3,19.8],[20.9,20.2,19.8],[21.0,20.2,19.8],[20.6,20.0,19.8],[20.0,20.3,20.4]])

# 2D補間
# https://docs.scipy.org/doc/scipy/reference/tutorial/interpolate.html#id1
XI, YI = np.meshgrid(np.arange(0, 15.1, 0.1), np.arange(0, 10.1, 0.1))
rbf = Rbf(x, y, z, function='multiquadric', epsilon=9.0, smooth=0)
#rbf = Rbf(x, y, z, epsilon=2)
ZI = rbf(XI, YI)
#tck = interpolate.bisplrep(x, y, z, s=0)
#ZI= interpolate.bisplev(XI[:,0], YI[0,:], tck)
plt.pcolor(XI, YI, ZI, cmap=cm.jet)
#plt.pcolor(x, y, z, cmap=cm.jet)
plt.scatter(x, y, 100, z, cmap=cm.jet, linewidths=1, edgecolors="black")
#plt.imshow(np.sqrt(x ** 2 + y ** 2), cmap=cm.jet)
#plt.imshow(z, cmap=cm.jet)
#print(ZI)


# 参考 http://seesaawiki.jp/met-python/d/matplotlib
#plt.title("Temperature [${}^\circ$C]", fontsize=16)
plt.title("Humidity [%]", fontsize=16)
plt.axis([1,15,1,10])
#plt.locator_params(axis='both', tight=True, nbins=10)
plt.colorbar()
plt.savefig('matplotlib.png', transparent=True, dpi=80)



