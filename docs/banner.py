# Crossings banner: wavesets drawn with WsGen's shape/skew code, parameters from seeded brownian walks.
import math, random, sys
random.seed(int(sys.argv[2]) if len(sys.argv) > 2 else 14)
W, H, MID = 1240, 320, 160.0
X0, X1 = 50.0, 1190.0
COL = "#5bc0be"
def shapeAt(k,p):
    if k==0: return math.sin(math.pi*p)
    if k==1: return 1-abs(2*p-1)
    if k==2: return 1.0
    return math.sin(math.pi*p)**8
def half(sh,p):
    k=int(sh); f=sh-k; a=shapeAt(k,p)
    return a+f*(shapeAt(min(k+1,3),p)-a) if f>0 else a
def walk(v,step,lo,hi):
    v+=random.uniform(-step,step)
    return lo+(hi-lo-abs((v-lo)%(2*(hi-lo))-(hi-lo))) if not lo<=v<=hi else v
seq=[]; ln, amp, sh, sk = 0.0, .5, 0.5, .5
while len(seq)<12:
    ln=walk(ln,.9,-2.2,1.4); amp=walk(amp,.35,.08,1.0); sh=walk(sh,.9,0,3); sk=walk(sk,.3,.08,.92)
    reps=random.choice([1,1,1,2])
    seq+= [(math.exp(ln),amp,sh,sk)]*reps
total=sum(s[0] for s in seq); scale=(X1-X0)/total
out=[f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" width="{W}" height="{H}" role="img">',
     f'  <rect width="{W}" height="{H}" fill="#000000"/>',
     f'  <line x1="30" y1="{MID}" x2="1210" y2="{MID}" stroke="#1c1c1c" stroke-width="2"/>']
x=X0; marks=[]
for (ln,amp,sh,sk) in seq:
    w=ln*scale; N=max(6,int(w*1.5)); pts=[]
    for j in range(N+1):
        p=j/N
        y=half(sh,p/sk) if p<sk else -half(sh,(p-sk)/(1-sk))
        if j in (0,N): y=0.0
        pts.append((x+p*w, MID-y*amp*110))
    d="M "+" L ".join(f"{a:.1f} {b:.1f}" for a,b in pts)
    out.append(f'  <path d="{d}" fill="none" stroke="{COL}" stroke-width="1.3" stroke-linejoin="miter" stroke-miterlimit="2"/>')
    marks.append(x)
    x+=w
out+= [f'  <circle cx="{m:.1f}" cy="{MID}" r="2" fill="#d8d8de"/>' for m in marks+[x]]
out.append('</svg>')
open(sys.argv[1],'w').write("\n".join(out)+"\n")
