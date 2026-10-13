#!/usr/bin/env python3
import argparse,hashlib,os,struct,sys
SHA256='ffe013d6067025281bc3c9bc9f9b62c99107baafdfb1005c2e98ae6dbc74714d'
CODE_VA=0x00158d00
HERE=os.path.dirname(os.path.abspath(__file__))
def die(s):sys.exit('xonline patch: '+s)
class Xbe:
 def __init__(self,b):self.b=b;self.base,self.nsec,self.st=struct.unpack_from('<Ixx',b,0x104)[0],struct.unpack_from('<I',b,0x11c)[0],struct.unpack_from('<I',b,0x120)[0]
 def secs(self):
  for i in range(self.nsec):
   o=self.st-self.base+i*0x38;f,v,z,r,rz=struct.unpack_from('<5I',self.b,o);yield f,v,z,r,rz
 def off(self,va):
  for f,v,z,r,rz in self.secs():
   if v<=va<v+rz:return r+va-v
  die(f'address {va:08x} is not mapped')
def main():
 ap=argparse.ArgumentParser();ap.add_argument('input');ap.add_argument('output');a=ap.parse_args()
 if os.path.abspath(a.input)==os.path.abspath(a.output):die('input and output must differ')
 b=bytearray(open(a.input,'rb').read())
 if hashlib.sha256(b).hexdigest()!=SHA256:die('unsupported xonlinedash.xbe')
 old_font=b'\\Device\\Harddisk0\\partition2\\xodash\\Xbox.xtf\0'
 new_font=b'\\Device\\Harddisk0\\partition2\\Xbox.xtf\0'
 font_off=0x3ecc
 if b[font_off:font_off+len(old_font)]!=old_font:die('font path mismatch at file offset 00003ecc')
 b[font_off:font_off+len(old_font)]=new_font+bytes(len(old_font)-len(new_font))
 x=Xbe(b)
 blob=open(os.path.join(HERE,'bin','xonline_skin.bin'),'rb').read();syms={}
 for line in open(os.path.join(HERE,'bin','symbols.txt')):
  p=line.split();syms[p[2]]=int(p[0],16)
 cave=x.off(CODE_VA)
 if any(b[cave:cave+len(blob)]):die('payload cave is not empty')
 b[cave:cave+len(blob)]=blob
 for va,old,sym,opcode in [(0x51247,'e87da10200','patch_xonline_return',0xe8),(0x99cb2,'d9442404d9','patch_xonline_rgba',0xe9),(0x99e2a,'8b44240489','patch_xonline_colors',0xe9),(0x9fc7c,'e8ab110000','patch_xonline_xbx_size',0xe8),(0x9fd17,'e8e0110000','patch_xonline_xbx_open',0xe8),(0x4d727,'e8b48f0900','patch_xonline_video_device',0xe9),(0x98ea2,'ff35c43519','patch_xonline_video_ui',0xe9)]:
  o=x.off(va)
  if b[o:o+5]!=bytes.fromhex(old):die(f'hook mismatch at {va:08x}')
  b[o:o+5]=bytes([opcode])+struct.pack('<I',(syms[sym]-va-5)&0xffffffff)
 open(a.output,'wb').write(b);print(a.output,hashlib.sha256(b).hexdigest())
if __name__=='__main__':main()
