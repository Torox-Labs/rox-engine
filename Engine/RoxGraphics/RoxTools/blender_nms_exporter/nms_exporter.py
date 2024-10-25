#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

tc1_as_tc0_zw = True

import bpy
from nya.nya_nms import nms_mesh

def export_nms(context, props, filepath):
    mesh = nms_mesh()

    tc_count = 0

    for obj in context.scene.objects:
        if obj.type == 'MESH':
            mesh_tc_count = len(obj.data.uv_textures)
            if mesh_tc_count > tc_count:
                tc_count = mesh_tc_count

    materials = []

    for obj in context.scene.objects:
        if obj.type == 'MESH':
            me = obj.data

            group = nms_mesh.nms_group()

            mats = me.materials
            if not mats:
                print("No materials found in mesh", me.name)
                continue

            if len(mats) > 1:
                print("More than 1 material per mesh is not supported", me.name)
                return

            group.mat_idx = mesh.add_material(mats[0].name)
            if group.mat_idx >= len(materials):
                materials.append(mats[0])

            if not me.tessfaces and me.polygons:
                me.calc_tessface()

            group.name = obj.name
            group.offset = mesh.vcount

            uv_sets = me.tessface_uv_textures
            has_tc = len(uv_sets) > 0
            has_tc2 = len(uv_sets) > 1
            blank_tc = tc_count > 0 and not has_tc
            blank_tc2 = tc_count > 1 and not has_tc2

            tc_inds = [0,1,2,3,0,2]

            matrix = obj.matrix_world.copy()
            for face in me.tessfaces:
                inds = []
                for i in face.vertices:
                    inds.append(i)

                #triangulate quad
                if len(inds) == 4:
                    inds.append(inds[-4]) #0
                    inds.append(inds[-3]) #2

                if has_tc:
                    face_uv = uv_sets[0].data[face.index].uv
                if has_tc2:
                    face_uv2 = uv_sets[1].data[face.index].uv

                for i,vi in enumerate(inds):
                    v = matrix * me.vertices[vi].co
                    mesh.verts_data.append(v[0])
                    mesh.verts_data.append(v[2])
                    mesh.verts_data.append(-v[1])

                    tci=tc_inds[i];

                    if has_tc:
                        mesh.verts_data.append(face_uv[tci][0])
                        mesh.verts_data.append(face_uv[tci][1])
                    elif blank_tc:
                        mesh.verts_data.append(0.0)
                        mesh.verts_data.append(0.0)

                    if has_tc2:
                        mesh.verts_data.append(face_uv2[tci][0])
                        mesh.verts_data.append(face_uv2[tci][1])
                    elif blank_tc2:
                        mesh.verts_data.append(0.0)
                        mesh.verts_data.append(0.0)

                    n = me.vertices[vi].normal.to_4d()
                    n.w = 0
                    n = (matrix * n).to_3d().normalized()
                    mesh.verts_data.append(n[0])
                    mesh.verts_data.append(n[2])
                    mesh.verts_data.append(-n[1])

                    #ToDo: skining

                mesh.vcount += len(inds)

            group.count = mesh.vcount - group.offset
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

    #ToDo: skining

    for i,m in enumerate(mesh.materials):
        m.params.append(nms_mesh.nms_param("nya_material", "default.txt"))
        for mtex in materials[i].texture_slots:
            if not mtex or not mtex.texture or mtex.texture.type != 'IMAGE':
                continue
            image = mtex.texture.image
            if not image:
                continue
            filename = bpy.path.basename(image.filepath)
            if mtex.use_map_color_diffuse:
                m.textures.append(nms_mesh.nms_param("diffuse", filename))
            if mtex.use_map_normal:
                m.textures.append(nms_mesh.nms_param("bump", filename))
            if mtex.use_map_ambient:
                m.textures.append(nms_mesh.nms_param("ambient", filename))
            if mtex.use_map_color_spec:
                m.textures.append(nms_mesh.nms_param("specular", filename))

    #ToDo: skeleton

    mesh.create_indices()
    mesh.write(filepath)
