#nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

bl_info = {
    "name": "nya-engine mesh exporter",
    "author": "nyan.developer@gmail.com",
    "version": (0, 1),
    "blender": (2, 5, 7),
    "location":     "File > Export",
    "description":  "Export nya-engine nms mesh",
    "category":     "Import-Export"
}

import bpy
from bpy_extras.io_utils import ExportHelper
from blender_nms_exporter.nms_exporter import *

class nms_exporter(bpy.types.Operator, ExportHelper):
    bl_idname       = "export.nms"
    bl_label        = "Export"
    bl_options      = {'PRESET'}

    filename_ext    = ".nms";

    def execute(self, context):
        props = self.properties
        filepath = self.filepath
        filepath = bpy.path.ensure_ext(filepath, self.filename_ext)
        export_nms(context, props, filepath)
        return {'FINISHED'}

def menu_func(self, context):
    self.layout.operator(nms_exporter.bl_idname, text="nya-engine mesh (.nms)")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
    register()
