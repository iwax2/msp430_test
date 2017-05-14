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
data = np.loadtxt("assmann_20170302.txt", delimiter="\t", dtype=np.float64, usecols=(0,1,2)) # , filling_values=(0, 0, 0) 

# 参考 http://www.mwsoft.jp/programming/numpy/csv.html
x = data[:,0]
y = data[:,1]
z = data[:,2]

print(x)
print(y)
print(z)
print(len(x),len(y),len(z))

# 2次元のスプライン補間
# https://docs.scipy.org/doc/scipy/reference/tutorial/interpolate.html#id1
XI, YI = np.meshgrid(np.arange(0, 15, 0.1), np.arange(0, 10, 0.1))
rbf = Rbf(x, y, z, function='gaussian', smooth=5.0)
ZI = rbf(XI, YI)
#f = interpolate.interp2d(x, y, z, kind='cubic')
#ZI = f(XI, YI)
plt.pcolor(XI, YI, ZI, cmap=cm.jet)
plt.scatter(x, y, 100, z, cmap=cm.jet, linewidths=1, edgecolors="black")
#plt.imshow(np.sqrt(x ** 2 + y ** 2), cmap=cm.jet)
#plt.imshow(z, cmap=cm.jet)



# 参考 http://seesaawiki.jp/met-python/d/matplotlib
plt.title("Temperature [${}^\circ$C]", fontsize=16)
plt.axis([1,15,1,10])
#plt.locator_params(axis='both', tight=True, nbins=10)
plt.colorbar()
plt.savefig('matplotlib.png', transparent=True, dpi=80)



