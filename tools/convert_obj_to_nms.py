#!/usr/bin/env python

#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

import sys
import os
from operator import itemgetter
from nya.nya_nms import *

class obj_mesh:
    class obj_material:
        def __init__(self):  
            self.params = {}
            self.textures = {}

    def __init__(self):
        self.vertices = []
        self.normals = []
        self.tcs = []
        self.faces = []
        self.materials = {}

    def load(self, filename):
        mat = None
        for line in open(filename, "r"):
            if line.startswith('#'): continue
            values = line.split()
            if not values: continue
            if values[0] == 'v':
                v = map(float, values[1:4])
                self.vertices.append(v)
            elif values[0] == 'vt':
                self.tcs.append(map(float, values[1:3]))
            elif values[0] == 'vn':
                v = map(float, values[1:4])
                self.normals.append(v)
            elif values[0] == 'f':
                v = []
                tc = []
                n = []
                for val in values[1:]:
                    w = val.split('/')
                    v.append(int(w[0]))
                    if len(w) >= 2 and len(w[1]) > 0:
                        tc.append(int(w[1]))
                    else:
                        tc.append(0)
                    if len(w) >= 3 and len(w[2]) > 0:
                        n.append(int(w[2]))
                    else:
                        n.append(0)
                self.faces.append((v, n, tc, mat))
            elif values[0] in ('usemtl', 'usemat'):
                mat = values[1]
            elif values[0] == 'mtllib':
                curr_mat = ""
                path = os.path.dirname(filename)
                path = os.path.join(path, '')
                for line in open(path + values[1], "r"):
                    if line.startswith('#'): continue
                    values = line.split()
                    if not values: continue
                    if values[0] == 'newmtl':
                        curr_mat = values[1]
                        self.materials[curr_mat] = obj_mesh.obj_material()
                    elif values[0].startswith('map_'):
                        self.materials[curr_mat].textures[values[0]] = values[1]
                    else:
                        self.materials[curr_mat].params[values[0]] = map(float, values[1:])

if len(sys.argv) < 3:
    print "please specify src and dst model names"
    print "convert_obj_to_nms src_name.obj dst_name.nms"
    exit(0)

print 'Converting ',sys.argv[1],' to ',sys.argv[2]

obj = obj_mesh()
obj.load(sys.argv[1])

obj.faces.sort(key = itemgetter(-1))

out = nms_mesh()
if len(obj.vertices) > 0:
    a = nms_mesh.nms_vertex_attribute()
    a.type = 0
    a.dimension = 3
    a.semantics = "pos"
    out.vert_attr.append(a);

has_tcs = len(obj.tcs) > 0
if len(obj.tcs) > 0:
    a = nms_mesh.nms_vertex_attribute()
    a.type = 100
    a.dimension = 2
    a.semantics = "tc0"
    out.vert_attr.append(a);

has_normals = len(obj.normals) > 0
if has_normals > 0:
    a = nms_mesh.nms_vertex_attribute()
    a.type = 1
    a.dimension = 3
    a.semantics = "normal"
    out.vert_attr.append(a);

last_mat = None
for i in range(len(obj.faces)):
    v, n, tc, mat = obj.faces[i]
    if last_mat != mat:
        if len(out.groups) > 0:
            out.groups[-1].count = out.vcount - out.groups[-1].offset
        g = nms_mesh.nms_group()
        g.name = mat
        g.offset = i*3
        g.mat_idx = [i for i, m in enumerate(obj.materials) if m==mat][0]
        out.groups.append(g)
        last_mat = mat

    for j in range(len(v)-2):
        for k in [0, j + 1, j + 2]:
            out.verts_data += obj.vertices[v[k]-1]
            if has_tcs:
                out.verts_data += obj.tcs[tc[k]-1]
            if has_normals:
                out.verts_data += obj.normals[n[k]-1]
        out.vcount = out.vcount + 3

if len(out.groups) > 0:
    out.groups[-1].count = int(out.vcount - out.groups[-1].offset)

for mat_name, mat in obj.materials.iteritems():
    m = nms_mesh.nms_material()
    m.name = mat_name
    p = nms_mesh.nms_param("nya_material")
    p.value = "obj.txt" #ToDo
    m.params.append(p)
    for n, t in mat.textures.iteritems():
        p = nms_mesh.nms_param()
        p.name = n
        p.value = t
        m.textures.append(p)

    for n, v in mat.params.iteritems():
        p = nms_mesh.nms_vec_param()
        p.name = n
        vl = len(v)
        if vl > 0:
            p.x = v[0]
        if vl > 1:
            p.y = v[1]
        if vl > 2:
            p.z = v[2]
        if vl > 3:
            p.w = v[3]
        m.vec_params.append(p)

    out.materials.append(m)

out.write(sys.argv[2])
