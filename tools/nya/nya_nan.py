#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

from nya.bin_data import *
from nya.nya_math import *

class nan_animation:
    class pos_vec3_linear_frame:
        def __init__( self ):
            self.time = 0
            self.pos = nya_vec3()

    class rot_quat_linear_frame:
        def __init__( self ):
            self.time = 0
            self.rot = nya_quat()

    class float_linear_frame:
        def __init__( self ):
            self.time = 0
            self.value = 0.0

    class bone_frames:
        def __init__( self ):
            self.name = ""
            self.frames = []

    def __init__( self ):
        self.pos_vec3_linear_curves = []
        self.rot_quat_linear_curves = []
        self.float_linear_curves = []

    def write(self,file_name):
        fo = open(file_name,"wb")
        if fo == 0:
            print("unable to open file ", file_name)
            return

        out = bin_data()
        out.add_data(("nya anim").encode())
        out.add_uint(1) #version

        out.add_uint(len(self.pos_vec3_linear_curves))
        for c in self.pos_vec3_linear_curves:
            out.add_string(c.name)
            out.add_uint(len(c.frames))
            for f in c.frames:
                out.add_uint(f.time)
                out.add_float(f.pos.x)
                out.add_float(f.pos.y)
                out.add_float(f.pos.z)

        out.add_uint(len(self.rot_quat_linear_curves))
        for c in self.rot_quat_linear_curves:
            out.add_string(c.name)
            out.add_uint(len(c.frames))
            for f in c.frames:
                out.add_uint(f.time)
                out.add_float(f.rot.v.x)
                out.add_float(f.rot.v.y)
                out.add_float(f.rot.v.z)
                out.add_float(f.rot.w)

        out.add_uint(len(self.float_linear_curves))
        for c in self.float_linear_curves:
            out.add_string(c.name)
            out.add_uint(len(c.frames))
            for f in c.frames:
                out.add_uint(f.time)
                out.add_float(f.value)

        fo.write(out.data)

        print("saved animation: ", file_name)
        print("file size: ", len(out.data)/1024, "kb")
