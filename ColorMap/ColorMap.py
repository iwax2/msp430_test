#!/usr/bin/python
# -*- coding: utf-8 -*-

# カラーマップを表示する

import matplotlib
matplotlib.rcParams['text.latex.unicode'] = True
matplotlib.use("Agg") # サーバサイドでXを不要にする
import matplotlib.pyplot as plt # 描画用
import numpy as np # 数値計算
import matplotlib.cm as cm # カラーバーの設定

x, y = np.meshgrid(np.arange(-3.0, 3.1, 0.1), np.arange(-3.0, 3.1, 0.1))
plt.title("Temperature [${}^\circ$C]", fontsize=16)
plt.imshow(np.sqrt(x ** 2 + y ** 2), cmap=cm.jet)
plt.colorbar()
plt.savefig('example.png', transparent=True, dpi=80)



