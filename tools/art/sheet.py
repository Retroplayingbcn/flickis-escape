import sys
from PIL import Image, ImageDraw
def md(c): return tuple(min(7,round(v/255*7))*36 for v in c)
def draw(art,pal,scale=8,bg=(200,200,200)):
    h=len(art); w=len(art[0])
    for r in art: assert len(r)==w,(r,len(r),w)
    im=Image.new('RGB',(w,h),bg); p=im.load()
    for y,row in enumerate(art):
        for x,ch in enumerate(row):
            if ch!='.': p[x,y]=md(pal[ch])
    return im.resize((w*scale,h*scale),Image.NEAREST)
