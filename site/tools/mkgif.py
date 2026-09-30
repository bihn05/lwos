import math, random
from PIL import Image, ImageDraw, ImageFont
import os
HERE=os.path.dirname(os.path.abspath(__file__))
S=os.path.join(HERE,'fonts/')
O=os.path.join(HERE,'../public/assets/gif/'); os.makedirs(O,exist_ok=True)
W95=ImageFont.truetype(S+'w95.ttf',16); FX=ImageFont.truetype(S+'fsex.ttf',16); W12=ImageFont.truetype(S+'w95.ttf',12)
LOGO=Image.open(os.path.join(HERE,'../public/assets/img/lwos.png')).convert('RGB')
def save(frames,name,dur,loop=0):
    fr=[f.convert('P',palette=Image.ADAPTIVE,colors=64) if f.mode!='P' else f for f in frames]
    fr[0].save(O+name,save_all=True,append_images=fr[1:],duration=dur,loop=loop,disposal=1,optimize=False)
def D(im): d=ImageDraw.Draw(im); d.fontmode='1'; return d

# 1. boot screen, 640x400 text mode
lines=["P0 ACTIVE; FAT32, BEG LBA AT 00000800.0003F800","NUM_FATS 00000002","FATS_SIZE 000003F1","ROOT CLUSTER: 00000002",
"FILE: ABI.BIN  CLUSTER AT 00000003  LENGTH 00001A40","FILE: MONITOR.BIN  CLUSTER AT 00000005  LENGTH 00003C80","END","",
"LWOS MONITOR v2 COPYLEFT 2026"]
script=[]  # list of (screen_lines, cursor, delay)
buf=[]
for l in lines:
    buf.append(l); script.append((list(buf),False,220 if l else 120))
def typed(cmd,out):
    global buf
    buf.append('>'); script.append((list(buf),True,500)); script.append((list(buf),False,300))
    for ch in cmd:
        buf[-1]+=ch; script.append((list(buf),True,160))
    for o in out: buf.append(o)
    script.append((list(buf),False,500))
typed('h',['d [ADDR]'])
typed('v',['GFX OFF ','FRAMEBUF=E0000000 0500x0400 20BIT PITCH=00000500*DWORD'])
buf.append('>')
for i in range(6): script.append((list(buf),i%2==0,450))
frames=[];durs=[]
GRY=(170,170,170)
for ls,cur,dl in script:
    im=Image.new('RGB',(640,288),(0,0,0)); d=D(im)
    for i,l in enumerate(ls): d.text((0,i*16),l,font=FX,fill=GRY)
    if True:
        # blinking underline cursor at end of last line
        if cur or ls[-1].startswith('>') and False: pass
        if cur:
            x=len(ls[-1])*8; y=(len(ls)-1)*16
            d.rectangle([x,y+13,x+7,y+14],fill=GRY)
    frames.append(im); durs.append(dl)
durs[-1]=2500
fr=[f.convert('P',palette=Image.ADAPTIVE,colors=4) for f in frames]
fr[0].save(O+'boot.gif',save_all=True,append_images=fr[1:],duration=durs,loop=0,optimize=False)

# 2. spinning globe 40x40
random.seed(7); MW,MH=96,48
m=Image.new('L',(MW,MH),0); md=ImageDraw.Draw(m)
for _ in range(14):
    x,y=random.randint(0,MW),random.randint(6,MH-8); r=random.randint(4,9)
    for dx in (-MW,0,MW): md.ellipse([x+dx-r,y-r*0.7,x+dx+r,y+r*0.7],fill=255)
fr=[]
N=24;R=19;C=20
for k in range(N):
    im=Image.new('RGB',(40,40),(192,192,192))
    px=im.load()
    for yy in range(40):
        for xx in range(40):
            dx=(xx-C+0.5)/R; dy=(yy-C+0.5)/R
            if dx*dx+dy*dy>1: continue
            z=math.sqrt(1-dx*dx-dy*dy)
            lon=math.atan2(dx,z)+k*2*math.pi/N; lat=math.asin(dy)
            u=int((lon/(2*math.pi))%1*MW); v=min(MH-1,int((lat/math.pi+0.5)*MH))
            shade=0.45+0.55*max(0,(-0.5*dx-0.5*dy+0.7*z))
            if m.getpixel((u,v))>0: c=(40,160,40)
            else: c=(30,90,220)
            q=lambda a:int(min(255,a*shade)//48*48+ (0 if shade<1 else 0))
            px[xx,yy]=tuple(q(a) for a in c)
    d=ImageDraw.Draw(im); d.ellipse([C-R,C-R,C+R-1,C+R-1],outline=(0,0,0))
    fr.append(im)
save(fr,'globe.gif',90)

# 3. NEW! flasher 36x16
fr=[]
for k in range(4):
    bg,fg=[((255,0,0),(255,255,0)),((255,255,0),(255,0,0))][k%2]
    im=Image.new('RGB',(40,16),bg); d=D(im); d.text((3,-1),'NEW!',font=W95,fill=fg)
    if k in (1,2): 
        sx=[(36,2),(2,12)][k-1]; d.point([(sx[0],sx[1]),(sx[0]-1,sx[1]),(sx[0]+1,sx[1]),(sx[0],sx[1]-1),(sx[0],sx[1]+1)],fill=(255,255,255))
    fr.append(im)
save(fr,'new.gif',260)

# 4. divider tile 64x6 marching LWOS colours
cols=[(255,0,0),(0,0,255),(255,255,0),(0,200,0)]
fr=[]
for k in range(16):
    im=Image.new('RGB',(64,6)); px=im.load()
    for x in range(64):
        c=cols[((x+k)//4)%4]
        for y in range(6): px[x,y]=c if 1<=y<=4 else (0,0,0)
    fr.append(im)
save(fr,'divider.gif',70)

# 5. under construction: barrier with blinking beacons 140x44
fr=[]
for k in range(8):
    im=Image.new('RGB',(140,44),(192,192,192)); d=D(im); px=im.load()
    for y in range(18,34):
        for x in range(8,132):
            px[x,y]=(0,0,0) if ((x+y+k*2)//6)%2 else (255,216,0)
    d.rectangle([7,17,132,34],outline=(0,0,0))
    for lx in (16,122):
        d.rectangle([lx-2,34,lx+2,43],fill=(80,80,80))
    on=[k%4<2,k%4>=2]
    for i,bx in enumerate((22,116)):
        d.rectangle([bx-5,5,bx+5,16],fill=(255,140,0) if on[i] else (120,60,0),outline=(0,0,0))
        if on[i]:
            for a in range(0,360,45):
                r=math.radians(a); d.line([bx+7*math.cos(r),10+6*math.sin(r),bx+10*math.cos(r),10+9*math.sin(r)],fill=(255,200,0))
    fr.append(im)
save(fr,'construction.gif',130)

# 6. spinning logo 32x32 (coin spin)
fr=[]
for k in range(20):
    a=k*2*math.pi/20; s=math.cos(a); w=max(2,int(abs(s)*32))
    im=Image.new('RGB',(32,32),(192,192,192))
    src=LOGO if s>=0 else LOGO.transpose(Image.FLIP_LEFT_RIGHT).point(lambda v:v//2)
    im.paste(src.resize((w,32),Image.NEAREST),((32-w)//2,0)); fr.append(im)
save(fr,'spin.gif',70)

# 7. 88x31 buttons
def bevel(d,bg):
    d.rectangle([0,0,87,30],fill=bg); d.line([0,0,87,0],fill=(255,255,255)); d.line([0,0,0,30],fill=(255,255,255))
    d.line([0,30,87,30],fill=(0,0,0)); d.line([87,0,87,30],fill=(0,0,0))
def btn(name,frames,dur): save(frames,name,dur)
# a) LWOS NOW!
fr=[]
for k in range(6):
    im=Image.new('RGB',(88,31)); d=D(im); bevel(d,(0,0,128))
    im.paste(LOGO.resize((24,24),Image.NEAREST),(4,4))
    d.text((32,3),'LWOS',font=W12,fill=(255,255,255))
    d.text((32,16),'NOW!',font=W12,fill=[(255,255,0),(255,0,0),(0,255,0)][k%3])
    fr.append(im)
btn('btn-lwos.gif',fr,300)
# b) best viewed in bochs
im=Image.new('RGB',(88,31)); d=D(im); bevel(d,(192,192,192))
d.text((5,3),'BEST VIEWED',font=W12,fill=(0,0,0)); d.text((5,16),'IN',font=W12,fill=(0,0,0)); d.text((20,16),'BOCHS',font=W12,fill=(160,0,0))
btn('btn-bochs.gif',[im],1000)
# c) NO BIOS (blinking strike)
fr=[]
for k in range(2):
    im=Image.new('RGB',(88,31)); d=D(im); bevel(d,(0,0,0))
    d.text((8,9),'BIOS',font=W12,fill=(0,255,0)); d.text((46,9),'FREE',font=W12,fill=(255,255,0))
    if k: d.line([5,21,36,10],fill=(255,0,0),width=2)
    fr.append(im)
btn('btn-nobios.gif',fr,600)
# d) powered by make
fr=[]
for k in range(8):
    im=Image.new('RGB',(88,31)); d=D(im); bevel(d,(255,255,255))
    d.text((5,2),'POWERED BY',font=W12,fill=(0,0,0))
    d.text((4,14),'$ make run'[:4+min(k,6)],font=FX,fill=(0,0,128))
    if k%2==0: x=4+8*len('$ make run'[:4+min(k,6)]); d.rectangle([x,26,x+6,27],fill=(0,0,128))
    fr.append(im)
btn('btn-make.gif',fr,250)
# e) x86 32-bit protected mode
im=Image.new('RGB',(88,31)); d=D(im); bevel(d,(128,0,0))
d.text((5,3),'x86',font=W12,fill=(255,255,0)); d.text((30,3),'32-BIT',font=W12,fill=(255,255,255)); d.text((5,16),'PROT. MODE',font=W12,fill=(255,255,255))
btn('btn-x86.gif',[im],1000)
print('ok')
