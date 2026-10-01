"""从 FSF 官方的 88x31 GPLv3 按钮派生出像素化版本。

官方那张是抗锯齿的矢量风图形, 和站上手工画的 1995 年 GIF 摆在一起太"新"。
官方 GPL/AGPL/LGPL logo 属于公有领域 (来源和声明见
public/assets/img/LICENSE-gplv3-88x31-logo.txt), 所以改是允许的。

做法: 缩到一半再用 NEAREST 放大回去, 一个源像素变成 2x2 方块; 再压到 8 色
调色板, 顺便去掉抗锯齿的渐变。alpha 一路保留, 所以放在什么底色上都行。

    python3 tools/mkgpl.py

输入是仓库里那份官方 PNG, 不从网上重新下载。和 mkgif.py 一样: 别因为顺手
跑了一遍就把输出提交上去, Pillow 各版本的 resize/quantize 不保证逐字节一致。
"""
import os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
IMG = os.path.join(HERE, '../public/assets/img/')
SRC = IMG + 'gplv3-88x31.png'
OUT = IMG + 'gplv3-88x31-pixel.png'

im = Image.open(SRC).convert('RGBA')
chunky = im.resize((im.width // 2, im.height // 2), Image.BOX).resize(im.size, Image.NEAREST)
alpha = chunky.getchannel('A')
flat = chunky.convert('RGB').quantize(colors=8, method=Image.MEDIANCUT).convert('RGBA')
flat.putalpha(alpha)
flat.save(OUT)
print('ok', os.path.relpath(OUT, HERE))
