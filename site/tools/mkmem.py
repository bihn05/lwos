from PIL import Image, ImageDraw, ImageFont
import os
HERE=os.path.dirname(os.path.abspath(__file__))
S=os.path.join(HERE,'fonts/')
O=os.path.join(HERE,'../public/assets/gif/'); os.makedirs(O,exist_ok=True)
FX=ImageFont.truetype(S+'fsex.ttf',16); W12=ImageFont.truetype(S+'w95.ttf',12)
BG=(0,0,170); WH=(255,255,255); GR=(170,170,170); YE=(255,255,85); CY=(85,255,255); BK=(0,0,0); DG=(0,0,110)
W,H=560,336
NS=8
# memory blocks top(high addr) -> bottom(low), (addr,label,height,key)
blocks=[("FFFFFFFF","",4,None),("E0000000","FRAMEBUFFER",20,'fb'),("","",16,'gap'),
 ("","  (free)",30,'free'),("","TASK B  code+stack",30,'B'),("","TASK A  code+stack",30,'A'),
 ("00300000","RESMAN HEAP",0,'heap'),
 ("00200000","MONITOR.BIN",26,'M'),("00180000","RESMAN.BIN",20,'R'),("00100000","ABI.BIN",20,'abi'),
 ("000A0000","VGA / ROM",18,'rom'),("00000000","IVT BDA STAGE2 BINFO",24,'low')]
gdt=[("00","NULL",None),("08","CODE32 base 0 lim 4G",None),("10","DATA32 base 0 lim 4G",None),("18","CODE16 tramp",None),("20","DATA16 tramp",None),
     ("28","TSS  MONITOR",'M'),("30","TSS  TASK A",'A'),("38","TSS  TASK B",'B')]
order=['M','A','B']
frames=[]; tick=0x1F0
BX0,BX1=84,250
def draw(cur,sub,tick):
    im=Image.new('RGB',(W,H),BG); d=ImageDraw.Draw(im); d.fontmode='1'
    d.rectangle([0,0,W-1,17],fill=GR); d.text((6,0),"FLAT MODEL   CR0.PG=0   linear = physical",font=FX,fill=BK)
    # memory column
    y=28; pos={}
    d.text((BX0,y-2),"",font=FX,fill=WH)
    for addr,lab,h,k in blocks:
        if k=='heap':
            d.line([BX0-4,y,BX1,y],fill=YE); d.text((4,y-8),addr,font=FX,fill=YE); d.text((BX1+6,y-8),"",font=FX)
            continue
        if k=='gap':
            for xx in range(BX0,BX1,8):
                d.line([xx,y+5,xx+4,y+2],fill=GR); d.line([xx+4,y+2,xx+8,y+5],fill=GR)
                d.line([xx,y+12,xx+4,y+9],fill=GR); d.line([xx+4,y+9,xx+8,y+12],fill=GR)
            d.text((4,y-1),"",font=FX,fill=GR); y+=h; continue
        run = (k==cur)
        fill = {'A':(0,120,0),'B':(140,0,140),'M':(0,90,160),'free':DG}.get(k,(40,40,120))
        if run: fill={'A':(0,200,0),'B':(230,0,230),'M':(0,150,255)}[k]
        d.rectangle([BX0,y,BX1,y+h-1],fill=fill,outline=WH if k!='free' else GR)
        if h>=14 and lab: d.text((BX0+5,y+(h-16)//2),lab,font=FX if h>=16 else W12,fill=BK if run else WH)
        if addr: d.text((4,y+h-12 if h>4 else y-6),addr,font=W12,fill=GR)
        pos[k]=(y,h); y+=h
    # heap brace label
    hy=pos['free'][0]; hb=pos['A'][0]+pos['A'][1]
    d.text((4,hy+40),"resman",font=W12,fill=YE); d.text((4,hy+52),"heap",font=W12,fill=YE)
    d.text((BX0,y+4),"not to scale",font=W12,fill=GR)
    # running task writes straight into the framebuffer: fixed dotted path, a packet climbs it
    if cur in pos:
        ty,th=pos[cur]; fy,fh=pos['fb']; x0=BX1-10; top=fy+fh+5; bot=ty
        for yy in range(top,bot,4): d.line([x0,yy,x0,yy+1],fill=YE)
        d.polygon([(x0,fy+fh),(x0-3,top),(x0+3,top)],fill=YE)
        py=bot-3-int((bot-3-top)*sub/(NS-1))
        d.rectangle([x0-2,py-2,x0+2,py+2],fill=YE)
        d.rectangle([x0-2,ty+th-6,x0+2,ty+th-3],fill=YE)
    # GDT table
    GX=300; d.text((GX,24),"GDT",font=FX,fill=YE); d.text((GX+40,24),"(no LDT, no paging)",font=W12,fill=GR)
    gy=44; rowpos={}
    for sel,desc,k in gdt:
        sel_on = (k==cur)
        if sel_on: d.rectangle([GX-2,gy,W-8,gy+17],fill=WH)
        d.text((GX,gy),sel,font=FX,fill=BG if sel_on else CY)
        d.text((GX+28,gy),desc,font=FX,fill=BK if sel_on else WH)
        rowpos[k]=gy+8; gy+=19
    # arrow from TSS row to block
    if cur in pos:
        ty,th=pos[cur]; ry=rowpos[cur]
        pts=[(GX-4,ry),(BX1+18,ry),(BX1+18,ty+th//2),(BX1+2,ty+th//2)]
        for a,b in zip(pts,pts[1:]):
            d.line([a,b],fill=YE,width=1)
        ax,ay=BX1+2,ty+th//2; d.polygon([(ax,ay),(ax+5,ay-3),(ax+5,ay+3)],fill=YE)
    # bottom status
    d.rectangle([0,H-50,W-1,H-1],fill=DG)
    sel={'M':'0028','A':'0030','B':'0038'}[cur]
    d.text((8,H-48),"IRQ0 100Hz  TICK %08X" % tick,font=FX,fill=WH)
    d.text((8,H-30),"EOI -> LJMP %s:0  " % sel,font=FX,fill=YE)
    base={'M':0x00000,'A':0x40000,'B':0x80000}[cur]
    d.text((180,H-30),"MOV [E%07X],EAX" % (base+0x1400*sub),font=FX,fill=CY)
    d.text((360,H-48),"all ring 0, one address space",font=W12,fill=GR)
    d.text((360,H-30),"C pointer = DMA address",font=W12,fill=GR)
    return im
durs=[]
for cyc in range(2):
  for t in order:
    for sub in range(NS):
        frames.append(draw(t,sub,tick)); tick+=1; durs.append(140)
fr=[f.convert('P',palette=Image.ADAPTIVE,colors=32) for f in frames]
fr[0].save(O+'flatmem.gif',save_all=True,append_images=fr[1:],duration=durs,loop=0)
