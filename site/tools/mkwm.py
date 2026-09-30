from PIL import Image, ImageDraw, ImageFont
import os
HERE=os.path.dirname(os.path.abspath(__file__))
S=os.path.join(HERE,'fonts/')
O=os.path.join(HERE,'../public/assets/img/'); os.makedirs(O,exist_ok=True)
AB=ImageFont.truetype(S+'ab.ttf',30); W12=ImageFont.truetype(S+'w95.ttf',12)
LOGO=Image.open(os.path.join(HERE,'../public/assets/img/lwos.png')).convert('RGB')
def word(txt,font,fill=(0,0,0)):
    m=Image.new('L',(200,50),0); d=ImageDraw.Draw(m); d.fontmode='1'; d.text((12,4),txt,font=font,fill=255)
    bb=m.getbbox(); m=m.crop((bb[0]-2,bb[1],bb[2]+2,bb[3])); h=m.height
    sh=0.22; w=m.width+int(h*sh)+1
    m=m.transform((w,h),Image.AFFINE,(1,sh,-h*sh,0,1,0),resample=Image.NEAREST)
    return m
def lockup(slogan=True):
    wm=word('LWOS',AB)
    W=4+32+5+wm.width+4; H=4+32+2+3+(17 if slogan else 0)+4
    im=Image.new('RGB',(W,H),(255,255,255)); d=ImageDraw.Draw(im); d.fontmode='1'
    im.paste(LOGO,(4,4))
    y0=4+(32-wm.height)//2+1
    # 1px grey drop shadow, then black
    im.paste((128,128,128),(4+32+5+1,y0+1),wm); im.paste((0,0,0),(4+32+5,y0),wm)
    y=4+32+2; cols=[(255,0,0),(0,0,255),(224,192,0),(0,180,0)]; seg=(W-8)/4
    for i,c in enumerate(cols): d.rectangle([4+int(i*seg),y,4+int((i+1)*seg)-1,y+2],fill=c)
    if slogan:
        y+=5; d.rectangle([4,y,W-5,y+13],outline=(0,0,0))
        t='Every byte by hand.'; tw=d.textlength(t,font=W12)
        # fake italic: draw then shear
        tm=Image.new('L',(int(tw)+8,14),0); td=ImageDraw.Draw(tm); td.fontmode='1'; td.text((3,0),t,font=W12,fill=255)
        tm=tm.transform(tm.size,Image.AFFINE,(1,0,0,0,1,0),resample=Image.NEAREST)
        im.paste((0,0,0),((W-tm.width)//2,y+1),tm)
    return im
lockup().save(O+'wordmark.png'); lockup(False).save(O+'wordmark-mini.png')
print(Image.open(O+'wordmark.png').size, Image.open(O+'wordmark-mini.png').size)
