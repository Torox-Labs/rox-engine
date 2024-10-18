#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

import struct

class bin_data:
    data = bytes()
    def add_uchar(self,u):
        self.data += struct.pack('<B',u)
    def add_ushort(self,u):
        self.data += struct.pack('<H',u)
    def add_ushorts(self,us):
        self.data += struct.pack('<%sH' % len(us),*us)
    def add_int(self,i):
        self.data += struct.pack('<i',i)
    def add_uint(self,u):
        self.data += struct.pack('<I',u)
    def add_uints(self,us):
        self.data += struct.pack('<%sI' % len(us),*us)
    def add_float(self,f):
        self.data += struct.pack('<f',f)
    def add_floats(self,fs):
        self.data += struct.pack('<%sf' % len(fs),*fs)
    def add_data(self,d):
        fmt = '<' + str(len(d)) + "s"
        self.data += struct.pack(fmt,d)
    def add_string(self,s):
        self.add_ushort(len(s))
        fmt = '<' + str(len(s)) + "s"
        self.data += struct.pack(fmt,str(s).encode())
