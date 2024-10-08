#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#maya script path on mac is ~/Library/Preferences/Autodesk/maya/scripts

#ToDo: remove unnecessary frames

import maya.cmds as cmds
from nya.nya_nan import *

start_dir = cmds.workspace(query=1, rootDirectory=1)

def export():
    mask = start_dir + '*.nan'
    file_name = cmds.fileDialog(mode=1, directoryMask=mask)
    if file_name == '':
        return

    tm = cmds.currentTime(q=1)
    cmds.currentTime(0)

    class joint:
        def __init__(self):
            self.name = ""
            self.pos = nya_vec3()
            self.irot = nya_quat()
    joints = []

    def get_joint_idx(joints, name):
        if name == "":
            return -1
        for i in range(len(joints)):
            if name == joints[i].name:
                return i
        return -1

    for j in cmds.ls(type="joint"):
        jnt = joint()
        jnt.name = j
        p = cmds.xform(j, translation=1, q=1)
        jnt.pos.from_xyz(p[0], p[1], p[2])
        r = cmds.xform(j, rotation=1, q=1)
        jnt.irot.from_pyr(r[0], r[1], r[2])
        jnt.irot.w = -jnt.irot.w
        joints.append(jnt)

    maya_frame_time = 1000/24 #ToDo currentUnit time
    frames_count = int(cmds.playbackOptions(q=1, max=1))

    print("exporting ", len(joints), " bones ", frames_count, "frames")

    anim = nan_animation()

    for j in joints:
        pf = nan_animation.bone_frames()
        pf.name = j.name
        anim.pos_vec3_linear_curves.append(pf)
        rf = nan_animation.bone_frames()
        rf.name = j.name
        anim.rot_quat_linear_curves.append(rf)

    for i in range(frames_count):
        cmds.currentTime(i+1)
        for j in range(len(joints)):
            jnt = joints[j];
            pf = nan_animation.pos_vec3_linear_frame()
            pf.time = i*maya_frame_time
            p = cmds.xform(jnt.name, translation=1, q=1)
            pf.pos.from_xyz(p[0], p[1], p[2])
            pf.pos = jnt.irot.rotate(pf.pos - jnt.pos)
            anim.pos_vec3_linear_curves[j].frames.append(pf)

            rf = nan_animation.rot_quat_linear_frame()
            rf.time=i*maya_frame_time
            r = cmds.xform(jnt.name,rotation=1,q=1)
            rf.rot.from_pyr(r[0], r[1], r[2])
            rf.rot = jnt.irot * rf.rot
            anim.rot_quat_linear_curves[j].frames.append(rf)

    cmds.currentTime(tm)
    anim.write(file_name)

export()
