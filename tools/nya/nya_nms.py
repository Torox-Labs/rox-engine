#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

from nya.bin_data import *
from nya.nya_math import *

#----------------------------------------------------------------

class nms_container:
    class nms_chunk:
        def __init__( self ):
            self.type = 0
            self.data = bin_data()

    def __init__( self ):
        self.version = 0
        self.chunks = []

    def load( self, file_name ):
        f = open(file_name,"rb")
        if f == 0:
            print("unable to open file ", file_name)
            return

        if f.read(8) != "nya mesh":
            print("invalid nms file ", file_name)
            return

        def read_uint(f):
            return struct.unpack('I', f.read(4))[0]

        self.version = read_uint(f)
        count = read_uint(f)
        self.chunks = []
        for i in range(count):
            c = nms_container.nms_chunk()
            c.type = read_uint(f)
            size = read_uint(f)
            c.data = f.read(size)
            self.chunks.append(c)

    def write( self, file_name ):
        f = open(file_name,"wb")
        if f == 0:
            print("unable to open file ", file_name)
            return

        out = bin_data()
        out.add_data(("nya mesh").encode())
        out.add_uint(self.version)
        out.add_uint(len(self.chunks))
        for c in self.chunks:
            out.add_uint(c.type)
            out.add_uint(len(c.data))
            out.data += c.data

        f.write(out.data)

#----------------------------------------------------------------

class nms_mesh:
    class nms_vertex_attribute:
        def __init__( self ):
            self.type = 0 #pos=0 normal=1 color=2 tc=100
            self.dimension = 0
            self.semantics = "" #optional

    class nms_group:
        def __init__( self ):
            self.name = ""
            self.mat_idx = 0
            self.offset = 0
            self.count = 0

    class nms_param:
        def __init__( self, name = None, value = None ):
            self.name = name if name is not None else ""
            self.value = value if value is not None else ""

    class nms_vec_param:
        def __init__( self ):
            self.name = ""
            self.x = 0.0
            self.y = 0.0
            self.z = 0.0
            self.w = 0.0

    class nms_material:
        def __init__( self ):
            self.name = ""
            self.textures = []
            self.params = []
            self.vec_params = []

    class nms_joint:
        def __init__( self ):
            self.name = ""
            self.rot = nya_quat()
            self.pos = nya_vec3()
            self.parent = -1

    def get_joint_idx( self, name ):
        if name == "":
            return -1
        for i in range(len(self.joints)):
            if name == self.joints[i].name:
                return i
        return -1

    def sort_joints( self ):
        for i in range(len(self.joints)):
            had_sorted = False
            for j in range(len(self.joints)):
                p = self.joints[j].parent
                if p > j:
                    had_sorted = True
                    self.joints[j],self.joints[p] = self.joints[p],self.joints[j]
                    for k in range(len(self.joints)):
                        if self.joints[k].parent == j:
                            self.joints[k].parent = p
                        elif self.joints[k].parent == p:
                            self.joints[k].parent = j
            if had_sorted == False:
                break

    def create_indices(self):
        if self.indices or self.vcount == 0:
            return

        vstride = len(self.verts_data)//self.vcount
        new_verts = []
        new_vcount = 0
        vdict = {}

        for i in range(self.vcount):
            off = i*vstride
            v = self.verts_data[off:off+vstride]
            vt = tuple(v)
            f = vdict.get(vt)
            if f is not None:
                self.indices.append(f)
                continue

            self.indices.append(new_vcount)
            vdict[vt] = new_vcount
            new_verts += v
            new_vcount += 1

        self.verts_data = new_verts
        self.vcount = new_vcount

    def add_material(self, name):
        for i in range(len(self.materials)):
            if name == self.materials[i].name:
                return i

        m = self.nms_material()
        m.name = name
        self.materials.append(m)
        return len(self.materials) - 1

    def __init__( self ):
        self.verts_data = [] #floats 
        self.vcount = 0
        self.vert_attr = []
        self.indices = [] 
        self.groups = []
        self.materials = []
        self.joints = []

    def write(self, file_name):
        out = nms_container()
        out.version = 1  #ToDo: version 2

        mat_count = len(self.materials)
        jcount = len(self.joints)

        #---------------- mesh data -----------------

        if self.vcount > 0:
            buf = bin_data()

            for i in range(6): #ToDo: aabb
                buf.add_float(0)

            atr_count = len(self.vert_attr)
            buf.add_uchar(atr_count)
            for a in self.vert_attr:
                buf.add_uchar(a.type)
                buf.add_uchar(a.dimension)
                buf.add_string(a.semantics)

            buf.add_uint(self.vcount)
            buf.add_floats(self.verts_data)

            icount = len(self.indices)
            if icount>65535:
                buf.add_uchar(4) #uint indices
                buf.add_uint(icount)
                buf.add_uints(self.indices)
            elif icount>0:
                buf.add_uchar(2) #ushort indices
                buf.add_uint(icount)
                buf.add_ushorts(self.indices)
            else:
                buf.add_uchar(0) #no indices

            groups_count = len(self.groups)
            buf.add_ushort(1) #lods count
            buf.add_ushort(groups_count)
            for g in self.groups:
                buf.add_string(g.name)
                for j in range(6):
                    buf.add_float(0)
                buf.add_ushort(g.mat_idx)
                buf.add_uint(g.offset)
                buf.add_uint(g.count)

            c = nms_container.nms_chunk()
            c.type = 0 #mesh
            c.data = buf.data
            out.chunks.append(c)

        #-------------- materials data ---------------

        if mat_count > 0:
            buf = bin_data()
            buf.add_ushort(mat_count)
            for m in self.materials:
                buf.add_string(m.name)
                buf.add_ushort(len(m.textures))
                for tex in m.textures:
                    buf.add_string(tex.name)
                    buf.add_string(tex.value)

                params=m.params
                buf.add_ushort(len(params))
                for p in params:
                    buf.add_string(p.name)
                    buf.add_string(p.value)

                vec_params=m.vec_params
                buf.add_ushort(len(vec_params))
                for p in vec_params:
                    buf.add_string(p.name)
                    buf.add_float(p.x)
                    buf.add_float(p.y)
                    buf.add_float(p.z)
                    buf.add_float(p.w)

                buf.add_ushort(0) #ToDo: integer params

            c = nms_container.nms_chunk()
            c.type = 2 #materials
            c.data = buf.data
            out.chunks.append(c)

        #-------------- skeleton data ---------------

        if jcount > 0:
            buf = bin_data()
            buf.add_uint(jcount)
            for i in range(jcount):
                buf.add_string(self.joints[i].name)

                buf.add_float(self.joints[i].rot.v.x)
                buf.add_float(self.joints[i].rot.v.y)
                buf.add_float(self.joints[i].rot.v.z)
                buf.add_float(self.joints[i].rot.w)

                buf.add_float(self.joints[i].pos.x)
                buf.add_float(self.joints[i].pos.y)
                buf.add_float(self.joints[i].pos.z)

                buf.add_int(self.joints[i].parent)

            c = nms_container.nms_chunk()
            c.type = 1 #skeleton
            c.data = buf.data
            out.chunks.append(c)

        out.write(file_name)
