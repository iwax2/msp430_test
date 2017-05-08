#!/usr/bin/python
# -*- coding: utf-8 -*-

# カラーマップを表示する

import matplotlib
matplotlib.rcParams['text.latex.unicode'] = True
matplotlib.use("Agg") # サーバサイドでXを不要にする
import matplotlib.pyplot as plt # 描画用
import numpy as np # 数値計算
from scipy import interpolate # スプライン補間
import matplotlib.cm as cm # カラーバーの設定

# 参考 http://www.mwsoft.jp/programming/numpy/csv.html
data = np.loadtxt("assmann_20170302.txt", delimiter="\t", usecols=(0,1,2)) # , filling_values=(0, 0, 0) 

print(data)

# 参考 http://seesaawiki.jp/met-python/d/matplotlib
x, y = np.meshgrid(np.arange(0, 15, 0.1), np.arange(0, 10, 0.1))
plt.title("Temperature [${}^\circ$C]", fontsize=16)
plt.axis([0,15,0,10])
#plt.locator_params(axis='both', tight=True, nbins=10)

# 2次元のスプライン補間
# https://docs.scipy.org/doc/scipy/reference/tutorial/interpolate.html#two-dimensional-spline-representation-procedural-bisplrep
plt.imshow(np.sqrt(x ** 2 + y ** 2), cmap=cm.jet)
plt.colorbar()
plt.savefig('example.png', transparent=True, dpi=80)



