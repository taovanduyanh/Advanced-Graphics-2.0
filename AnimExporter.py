bl_info = {
	"name": "NCL Mesh Animation", 
	"category": "Import-Export",
	"blender": (2, 7, 8),
    "location": "File > Export > NCL Mesh Animation (.anm)",
    "description": "Export NCL Mesh Animation (.anm)"
}

import bpy, struct, math, os, time
from bpy.props import *
      
class ExportSettings:
  def __init__(self, filepath):
    self.filepath = filepath


def save_anm(settings):
  print("Saving as NCL Mesh Animation...")
  
  VPositions		  = 1 << 0
  VNormals			  = 1 << 1
  VTangents			  = 1 << 2
  VColors			    = 1 << 3
  VTex0				    = 1 << 4
  VTex1				    = 1 << 5
  VWeightValues		= 1 << 6
  VWeightIndices	= 1 << 7
  Indices			    = 1 << 8 
  JointNames      = 1 << 9  
  JointParents    = 1 << 10
  BindPose			  = 1 << 11
  BindPoseInv			= 1 << 12
  Material			  = 1 << 13
  SubMeshes       = 1 << 14
  
  if len(bpy.data.scenes) == 0:
    print("No scenes?!")
	
  sce = bpy.data.scenes[0]
  
  meshDatas = []
  allMeshes = []
  armature = None
	
  if len(sce.objects) == 0:
    print("No objects?!")
	
  for obj in sce.objects:
    if obj.type == 'ARMATURE':
      armature = obj
      print("Found armature object!")
      break

  if armature == None:
    print("No submeshes to export?!")
    return

  print("Mesh made from %i subMeshes" %(len(allMeshes)))
  
  allJointNames     = []
  allJointMatrices  = []
  allJointParents   = []
  allSkinData       = []


  frameCount = sce.frame_end - sce.frame_start;

  print("Exporting animation")
  out = open(settings.filepath, 'w')
  out.write('MeshAnim\n');
  out.write('%i\n' % 1)	#version
  out.write('%i\n' % frameCount) #frameCount
  out.write('%i\n' % len(armature.pose.bones)) #bonecount

  print("Animation has %i frames" %(frameCount))
  if armature != None:
    doSkins = True
    for bone in armature.pose.bones:
      allJointNames.append(bone.name)
      allJointMatrices.append(bone.matrix)
      i = -1
      if(bone.parent):
        i = 0
        for pBone in armature.pose.bones:
          if(pBone == bone.parent):
            break
          i = i + 1
      allJointParents.append(i)
 
  for f in range(sce.frame_start, sce.frame_end+1):
    sce.frame_set(f)
    print("Frame %i" % f)
    for bone in armature.pose.bones:
      mat = bone.matrix
      out.write( '%f %f %f %f\n' % (mat[0][0], mat[0][1], mat[0][2],mat[0][3] ))
      out.write( '%f %f %f %f\n' % (mat[1][0], mat[1][1], mat[1][2],mat[1][3] ))
      out.write( '%f %f %f %f\n' % (mat[2][0], mat[2][1], mat[2][2],mat[2][3] ))
      out.write( '%f %f %f %f\n' % (mat[3][0], mat[3][1], mat[3][2],mat[3][3] ))
      out.write( '\n' )

  out.close()
	
  print("Finished saving MeshAnim!")

class ExportMsh(bpy.types.Operator):
  '''Export to .anm'''
  bl_idname = "export.msh"
  bl_label = 'Export MeshAnim'
  filename_ext = ".anm"
    
  filepath    = StringProperty(name="File Path", description="Filepath for exporting", maxlen= 1024, default="")

  def execute(self, context):
    settings = ExportSettings(self.properties.filepath)
    save_anm(settings)
    return {'FINISHED'}
		
  def invoke(self, context, event):
    self.filepath = ""
    wm = context.window_manager
    wm.fileselect_add(self)
    return {'RUNNING_MODAL'}
		
  @classmethod
  def poll(cls, context):
    return context.active_object != None
		
def menu_func(self, context):
	self.layout.operator(ExportMsh.bl_idname, text="NCL MeshAnim", icon='EXPORT')
	
def register():
    print("Registering MeshAnim addon")
    bpy.utils.register_class(ExportMsh)
    bpy.types.INFO_MT_file_export.append(menu_func)
	
def unregister():
    print("Unregistering MeshAnim addon")
    bpy.utils.unregister_class(ExportMsh)
    bpy.types.INFO_MT_file_export.remove(menu_func)
	
if __name__ == "__main__":
    register()