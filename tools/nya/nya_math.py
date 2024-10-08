#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

import math

#----------------------------------------------------------------

class nya_vec3:
    def __init__(self):
        self.x = 0.0
        self.y = 0.0
        self.z = 0.0

    def from_xyz(self,x,y,z):
        self.x = x
        self.y = y
        self.z = z
        return self

    def __eq__(a, b):
        eps = 0.001
        if abs(a.x - b.x) > eps:
            return False
        if abs(a.y - b.y) > eps:
            return False
        if abs(a.z - b.z) > eps:
            return False
        return True

    def __add__(a, b):
        v = nya_vec3()
        v.x = a.x+b.x
        v.y = a.y+b.y
        v.z = a.z+b.z
        return v

    def __sub__(a, b):
        v = nya_vec3()
        v.x = a.x-b.x
        v.y = a.y-b.y
        v.z = a.z-b.z
        return v

    def __mul__(a, b):
        v = nya_vec3()
        v.x=a.x*b
        v.y=a.y*b
        v.z=a.z*b
        return v

    def __neg__(a):
        v = nya_vec3()
        v.x=-a.x
        v.y=-a.y
        v.z=-a.z
        return v

    def cross(a,b):
        v = nya_vec3()
        v.x = a.y*b.z - a.z*b.y
        v.y = a.z*b.x - a.x*b.z
        v.z = a.x*b.y - a.y*b.x
        return v

#----------------------------------------------------------------

class nya_quat:
    def __init__(self):
        self.v = nya_vec3()
        self.w = 1.0

    def from_pyr(self,pitch,yaw,roll):
        d2r = math.pi / 180.0
        hp = pitch * 0.5 * d2r
        hy = yaw * 0.5 * d2r
        hr = roll * 0.5 * d2r

        sx = math.sin(hp)
        cx = math.cos(hp)

        sy = math.sin(hy)
        cy = math.cos(hy)

        sz = math.sin(hr)
        cz = math.cos(hr)

        self.v = nya_vec3()

        self.v.x = sx*cy*cz - cx*sy*sz
        self.v.y = cx*sy*cz + sx*cy*sz
        self.v.z = cx*cy*sz - sx*sy*cz
        self.w = cx*cy*cz + sx*sy*sz
        return self

    def __eq__(a, b):
        eps = 0.001
        if abs(a.v.x - b.v.x) > eps:
            return False
        if abs(a.v.y - b.v.y) > eps:
            return False
        if abs(a.v.z - b.v.z) > eps:
            return False
        if abs(a.w - b.w) > eps:
            return False
        return True

    def __mul__(a, b):
        q = nya_quat()
        q.v.x = a.w*b.v.x + a.v.x*b.w   + a.v.y*b.v.z - a.v.z*b.v.y
        q.v.y = a.w*b.v.y - a.v.x*b.v.z + a.v.y*b.w   + a.v.z*b.v.x
        q.v.z = a.w*b.v.z + a.v.x*b.v.y - a.v.y*b.v.x + a.v.z*b.w
        q.w   = a.w*b.w   - a.v.x*b.v.x - a.v.y*b.v.y - a.v.z*b.v.z
        return q

    def rotate(self,v):
        return v + nya_vec3.cross(self.v, nya_vec3.cross(self.v, v) + v*self.w) * 2.0

    def rotate_inv(self,v):
        return v + nya_vec3.cross(nya_vec3.cross(v, self.v) + v*self.w, self.v) * 2.0
