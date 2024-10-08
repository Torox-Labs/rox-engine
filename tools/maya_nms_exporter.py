#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#maya script path on mac is ~/Library/Preferences/Autodesk/maya/scripts

#ToDo: fnDagNode.findPlug( 'visibility' ).asBool()


textures_cut_path = "textures/"
tc1_as_tc0_zw = True

import maya.cmds as cmds
import maya.OpenMaya as OpenMaya
import maya.OpenMayaAnim as OpenMayaAnim
from nya.nya_nms import *

start_dir = cmds.workspace(query=1, rootDirectory=1)

def export():
    mask = start_dir + '*.nms'
    file_name = cmds.fileDialog(mode=1, directoryMask=mask)
    if file_name == '':
        return

    tm = cmds.currentTime(q=1)
    cmds.currentTime(0)

    tri_list=[]
    for m in cmds.ls(type='mesh'):
        tri = cmds.polyTriangulate(m)[0]
        tri_list.append(tri)

    mesh = nms_mesh()

    #skeleton

    for j in cmds.ls(type="joint"):
        jnt = nms_mesh.nms_joint()
        jnt.name = j
        p = cmds.xform(j,absolute=1,translation=1,q=1,ws=1)
        jnt.pos.from_xyz(p[0],p[1],p[2])
        r = cmds.xform(j,absolute=1,rotation=1,q=1,ws=1)
        jnt.rot.from_pyr(r[0],r[1],r[2])
        mesh.joints.append(jnt)

    for j in mesh.joints:
        parents = cmds.listRelatives(j.name,parent=1)
        if parents:
            j.parent = mesh.get_joint_idx(parents[0])

    mesh.sort_joints()

    #mesh data

    maya_meshes = []
    itDag = OpenMaya.MItDag( OpenMaya.MItDag.kDepthFirst, OpenMaya.MFn.kTransform )
    while not itDag.isDone():
        path = OpenMaya.MDagPath()
        itDag.getPath( path )
        fnDagNode = OpenMaya.MFnDagNode( path )
        if not fnDagNode.isIntermediateObject():
            numChildren = fnDagNode.childCount()
            for i in range( numChildren ):
                child = fnDagNode.child( i )
                path.push( child )
                if path.node().apiType() == OpenMaya.MFn.kMesh:
                    m = OpenMaya.MFnMesh( path )
                    if not m.isIntermediateObject():
                        maya_meshes.append(m)
                path.pop()
        itDag.next()

    tc_count = 0
    need_skining = len(mesh.joints) > 0 #ToDo

    for m in maya_meshes:
        uv_sets = [];
        m.getUVSetNames(uv_sets)
        if len(uv_sets) > tc_count:
            tc_count = len(uv_sets)

    for m in maya_meshes:
        group = nms_mesh.nms_group()
        group.name = m.partialPathName()
        group.offset = mesh.vcount

        uv_sets = [];
        m.getUVSetNames(uv_sets)
        num_uv_sets = len(uv_sets)

        has_tc = len(uv_sets) > 0
        has_tc2 = len(uv_sets) > 1
        blank_tc = tc_count > 0 and not has_tc
        blank_tc2 = tc_count > 1 and not has_tc2

        vert_counts = OpenMaya.MIntArray()
        vert_inds = OpenMaya.MIntArray()
        m.getVertices(vert_counts, vert_inds)

        points = OpenMaya.MPointArray()
        m.getPoints(points, OpenMaya.MSpace.kWorld)

        tci_util = OpenMaya.MScriptUtil()
        tci_util.createFromInt(0)
        tci_ptr = tci_util.asIntPtr()

        tc_us = OpenMaya.MFloatArray()
        tc_vs = OpenMaya.MFloatArray()
        tc_inds = []
        if has_tc:
            m.getUVs(tc_us, tc_vs, uv_sets[0])
            for i in range(vert_counts.length()):
                for j in range(3):
                    m.getPolygonUVid(i, j, tci_ptr, uv_sets[0])
                    tc_inds.append(tci_util.getInt(tci_ptr))

        tc2_us = OpenMaya.MFloatArray()
        tc2_vs = OpenMaya.MFloatArray()
        tc2_inds = []
        if has_tc2:
            m.getUVs(tc2_us, tc2_vs, uv_sets[1])
            for i in range(vert_counts.length()):
                for j in range(3):
                    m.getPolygonUVid(i, j, tci_ptr, uv_sets[1])
                    tc2_inds.append(tci_util.getInt(tci_ptr))

        normals = OpenMaya.MFloatVectorArray()
        m.getNormals(normals, OpenMaya.MSpace.kWorld)

        normal_counts = OpenMaya.MIntArray()
        normal_inds = OpenMaya.MIntArray()
        m.getNormalIds(normal_counts,normal_inds)

        weights = OpenMaya.MDoubleArray()
        influences_count = 0
        influences = OpenMaya.MDagPathArray()
        joints_remap = []

        if need_skining:
            plugInMesh = m.findPlug( 'inMesh' )
            try:
                itDag = OpenMaya.MItDependencyGraph( plugInMesh, OpenMaya.MFn.kSkinClusterFilter, OpenMaya.MItDependencyGraph.kUpstream, OpenMaya.MItDependencyGraph.kDepthFirst, OpenMaya.MItDependencyGraph.kNodeLevel )

                while not itDag.isDone():
                    sc = OpenMayaAnim.MFnSkinCluster( itDag.currentItem() )

                    members = OpenMaya.MSelectionList()
                    OpenMaya.MFnSet( sc.deformerSet() ).getMembers( members, False )
                    dagPath = OpenMaya.MDagPath()
                    components = OpenMaya.MObject()
                    members.getDagPath( 0, dagPath, components )

                    util = OpenMaya.MScriptUtil()
                    util.createFromInt(0)
                    pNumInfluences = util.asUintPtr()
                    sc.getWeights( dagPath, components, weights, pNumInfluences )
                    util = OpenMaya.MScriptUtil( pNumInfluences )
                    influences_count = util.asUint()
                    sc.influenceObjects(influences)
                    break
            except:
                # no skin cluster found
                pass

            for i in range(influences.length()):
                joints_remap.append(mesh.get_joint_idx(influences[i].partialPathName()))

        for i in range(vert_inds.length()):
            vi = vert_inds[i]
            p = points[vi]
            mesh.verts_data.append(p.x)
            mesh.verts_data.append(p.y)
            mesh.verts_data.append(p.z)

            if has_tc:
                tci=tc_inds[i]
                mesh.verts_data.append(tc_us[tci])
                mesh.verts_data.append(tc_vs[tci])
            elif blank_tc:
                mesh.verts_data.append(0.0)
                mesh.verts_data.append(0.0)

            if has_tc2:
                tci=tc2_inds[i]
                mesh.verts_data.append(tc2_us[tci])
                mesh.verts_data.append(tc2_vs[tci])
            elif blank_tc2:
                mesh.verts_data.append(0.0)
                mesh.verts_data.append(0.0)

            n = normals[normal_inds[i]]
            mesh.verts_data.append(n.x)
            mesh.verts_data.append(n.y)
            mesh.verts_data.append(n.z)

            if need_skining:
                off = vi * influences_count
                count = 0
                ws = []
                ws_sum = 0.0
                for j in range(influences_count):
                    w = weights[off + j]
                    if w < 0.001:
                        continue
                    mesh.verts_data.append(joints_remap[j])
                    ws.append(w)
                    count += 1
                    ws_sum += w
                    if count > 4:
                        print("ERROR: Cannot export. Vertex invluence count > 4. Mesh", group.name)
                        cmds.delete(tri_list)
                        cmds.currentTime(tm)
                        return

                for j in range(count,4):
                    mesh.verts_data.append(0)

                if ws_sum > 0.001:
                    for w in ws:
                        mesh.verts_data.append(w/ws_sum)
                else:
                    count = 0

                for j in range(count,4):
                    mesh.verts_data.append(0.0)

        group.count = vert_inds.length()
        mesh.vcount += group.count

        shaders = OpenMaya.MObjectArray()
        face_mat_indices = OpenMaya.MIntArray()
        m.getConnectedShaders(0, shaders, face_mat_indices)

        #ToDo: face_mat_indices
        if shaders.length() > 1:
            cmds.delete(tri_list)
            cmds.currentTime(tm)
            print("More than 1 shader per mesh is not supported", group.name)
            return

        if shaders.length() == 0:
            print("No shaders found in mesh", group.name)
            cmds.delete(tri_list)
            cmds.currentTime(tm)
            return

        plugShader = OpenMaya.MFnDependencyNode(shaders[0]).findPlug('surfaceShader')
        mats = OpenMaya.MPlugArray()
        plugShader.connectedTo(mats, True, False)

        #ToDo: face_mat_indices
        if mats.length() > 1:
            cmds.delete(tri_list)
            cmds.currentTime(tm)
            print("More than 1 material per mesh is not supported", group.name)
            return

        if mats.length() == 0:
            cmds.delete(tri_list)
            cmds.currentTime(tm)
            print("No materials found in mesh", group.name)
            return

        at = mats[0].node().apiType()
        if at == OpenMaya.MFn.kPhong or at == OpenMaya.MFn.kBlinn or at == OpenMaya.MFn.kLambert or at == OpenMaya.MFn.kSurfaceShader:
            name = OpenMaya.MFnDependencyNode(mats[0].node()).name()
            group.mat_idx = mesh.add_material(name)
        else:
            cmds.delete(tri_list)
            cmds.currentTime(tm)
            print("Unsupported material in mesh", group.name)
            return

        mesh.groups.append(group)

    a = nms_mesh.nms_vertex_attribute()
    a.type = 0
    a.dimension = 3
    a.semantics = "pos"
    mesh.vert_attr.append(a);

    if tc_count > 0:
        a = nms_mesh.nms_vertex_attribute()
        a.type = 100
        if tc_count > 1 and tc1_as_tc0_zw:
            a.dimension = 4
            a.semantics = "tc0_tc1"
        else:
            a.dimension = 2
            a.semantics = "tc0"
        mesh.vert_attr.append(a);

        if tc_count > 1 and not tc1_as_tc0_zw:
            a = nms_mesh.nms_vertex_attribute()
            a.type = 100+1
            a.dimension = 2
            a.semantics = "tc1"
            mesh.vert_attr.append(a);

    a = nms_mesh.nms_vertex_attribute()
    a.type = 1
    a.dimension = 3
    a.semantics = "normal"
    mesh.vert_attr.append(a);

    if need_skining:
        a = nms_mesh.nms_vertex_attribute()
        a.type = 100+2
        a.dimension = 4
        a.semantics = "bone_idx"
        mesh.vert_attr.append(a);

        a = nms_mesh.nms_vertex_attribute()
        a.type = 100+3
        a.dimension = 4
        a.semantics = "bone_weight"
        mesh.vert_attr.append(a);

    mesh.create_indices()

    #materials

    def get_tex_name(mat_name, semantics):
        f = cmds.listConnections(mat_name + '.'+ semantics)
        if f and cmds.nodeType(f) == "file":
            tex = cmds.getAttr(f[0] + ".fileTextureName")
            idx = tex.find(textures_cut_path)
            if idx >= 0:
                tex = tex[idx + len(textures_cut_path):len(tex)]
            return tex
        return None

    for m in mesh.materials:
        attr = cmds.listAttr(m.name,ud=1)
        if attr:
            for a in attr:
                p = nms_mesh.nms_param(a, cmds.getAttr(m.name + "." + a))
                m.params.append(p)

        t = get_tex_name(m.name, "color")
        if t:
            m.textures.append(nms_mesh.nms_param("diffuse", t))

        t = get_tex_name(m.name, "ambientColor")
        if t:
            m.textures.append(nms_mesh.nms_param("ambient", t))

    cmds.delete(tri_list)
    cmds.currentTime(tm)
    mesh.write(file_name)

export()
