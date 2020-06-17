bl_info = {
	"name": "MeshGeometry", 
	"category": "Import-Export",
	"blender": (2, 7, 8),
    "location": "File > Export > NCL MeshGeometry (.msh)",
    "description": "Export mesh MeshGeometry (.msh)"
}

import bpy, struct, math, os, time
from bpy.props import *

def mesh_triangulate(me):
    mesh = me
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()

class vSkinEntry:
  weight = 0.0
  index = 0
	
class vSkinData:
  entries = []

  weights = []
  indices = []
  
  count = 0
  
  def __init__(self):
    self.weights = [0.0, 0.0, 0.0, 0.0]	
    self.indices = [0.0, 0.0, 0.0, 0.0]
    self.entries = []

  def AddData(self, index, weight):
    entry = vSkinEntry()
    entry.weight = weight
    entry.index = index
    self.entries.append(entry)

    if(self.count < 4): #we'll premptively fill up the results in case there's < 4 entries
      self.weights[self.count] = weight
      self.indices[self.count] = index
      self.count = self.count + 1
	
  def FinaliseData(self):
    if(len(self.entries) > 4): #we have to sort and reweight!
      self.entries = sorted(self.entries, key = lambda entry: entry.weight, reverse=True) ##sort into descending order

      weightAmount = 0
      for x in range(0,4):
        self.weights[x] = self.entries[x].weight
        self.indices[x] = self.entries[x].index
        weightAmount += self.entries[x].weight
		#Multiply up the chosen weights so they sum to 1
      weightRecip = 1.0 / weightAmount

      for x in range(0,4):
        self.weights[x] = self.weights[x] / weightAmount

class MeshData:
	start = 0
	end = 0


class Vector2D:
  uv = []
  def __init__(self, u = 0.0, v = 0.0):
    self.uv = [u, v]
  
  def SetValues(self, x,y):
    self.uv[0] = x  
    self.uv[1] = y


  def SetValues(self, other):
    self.uv[0] = other[0]	  
    self.uv[1] = other[1]

  def Print(self, stream):
      stream.write( '%f %f\n' % (self.uv[0], self.uv[1]) )

  def AddValues(self, other):
    self.uv[0] = self.uv[0] + other.uv[0]
    self.uv[1] = self.uv[1] + other.uv[1]

  def SubtractValues(self, other):
    self.uv[0] = self.uv[0] - other.uv[0]
    self.uv[1] = self.uv[1] - other.uv[1]

  def Magnitude(self):
    return math.sqrt( (self.uv[0] * self.uv[0]) + (self.uv[1] * self.uv[1]) )

  def Compare(self, other, threshold):
    diff = Vector2D()
    diff.SetValues(self.uv)
    diff.SubtractValues(other)
    if(diff.Magnitude() < threshold):
      return True
    return False

class Vector3D:
    xyz = []
	
    def __init__(self, x = 0.0, y = 0.0, z = 0.0):
      self.xyz = [x, y, z]
	    
    def SetValues(self, x,y,z):
      self.xyz[0] = x	  
      self.xyz[1] = y	
      self.xyz[2] = z

    def SetValues(self, other):
      self.xyz[0] = other[0]	  
      self.xyz[1] = other[1]
      self.xyz[2] = other[2]    
	  
    def AddValues(self, other):
      self.xyz[0] = self.xyz[0] + other.xyz[0]  
      self.xyz[1] = self.xyz[1] + other.xyz[1]
      self.xyz[2] = self.xyz[2] + other.xyz[2]

    def SubtractValues(self, other):
      self.xyz[0] = self.xyz[0] - other.xyz[0]
      self.xyz[1] = self.xyz[1] - other.xyz[1]
      self.xyz[2] = self.xyz[2] - other.xyz[2]

    def Normalise(self):
      l = math.sqrt( (self.xyz[0]*self.xyz[0]) + (self.xyz[1]*self.xyz[1]) + (self.xyz[2]*self.xyz[2]) )
      if l > 0.0:
        self.xyz[0] = self.xyz[0] / l
        self.xyz[1] = self.xyz[1] / l  
        self.xyz[2] = self.xyz[2] / l

    def Print(self, stream):
      stream.write( '%f %f %f\n' % (self.xyz[0], self.xyz[1], self.xyz[2] ) )

    def Magnitude(self):
      return math.sqrt( (self.xyz[0] * self.xyz[0]) + (self.xyz[1] * self.xyz[1]) + (self.xyz[2] * self.xyz[2]) )

    def Compare(self, other, threshold):
      diff = Vector3D()
      diff.SetValues(self.xyz)
      diff.SubtractValues(other)
      if(diff.Magnitude() < threshold):
        return True
      return False

    def CrossProduct(self, other):
      outVal = Vector3D()
      outVal.xyz[0] = ( self.xyz[1] * other.xyz[2] ) - ( self.xyz[2] * other.xyz[1] )
      outVal.xyz[1] = ( self.xyz[2] * other.xyz[0] ) - ( self.xyz[0] * other.xyz[2] )
      outVal.xyz[2] = ( self.xyz[0] * other.xyz[1] ) - ( self.xyz[1] * other.xyz[0] )
      return outVal
      
class Vector4D:
    rgba = []
	
    def __init__(self, x = 0.0, y = 0.0, z = 0.0, w = 0.0):
      self.rgba = [x, y, z, w]	
	  
    def SetValues(self, r, g, b, a):
      self.rgba[0] = r	  
      self.rgba[1] = g
      self.rgba[2] = b
      self.rgba[3] = a

    def SetValues(self, other):
      self.rgba[0] = other[0]	  
      self.rgba[1] = other[1]
      self.rgba[2] = other[2]    	
      self.rgba[3] = other[3]    

    def AddValues(self, other):
      self.rgba[0] = self.rgba[0] + other.rgba[0]
      self.rgba[1] = self.rgba[1] + other.rgba[1]
      self.rgba[2] = self.rgba[2] + other.rgba[2]
      self.rgba[3] = self.rgba[3] + other.rgba[3]

    def SubtractValues(self, other):
      self.rgba[0] = self.rgba[0] - other.rgba[0]
      self.rgba[1] = self.rgba[1] - other.rgba[1]
      self.rgba[2] = self.rgba[2] - other.rgba[2]
      self.rgba[3] = self.rgba[3] - other.rgba[3]

    def Print(self, stream):
      stream.write( '%f %f %f %f\n' % (self.rgba[0], self.rgba[1], self.rgba[2], self.rgba[3] ) )

    def Magnitude(self):
      return math.sqrt( (self.rgba[0] * self.rgba[0]) + (self.rgba[1] * self.rgba[1]) + (self.rgba[2] * self.rgba[2]) + (self.rgba[3] * self.rgba[3]) )

    def Compare(self, other, threshold):
      diff = Vector4D()
      diff.SetValues(self.rgba)
      diff.SubtractValues(other)
      if(diff.Magnitude() < threshold):
        return True
      return False

      
class ExportSettings:
  def __init__(self, filepath, triangulate):
    self.filepath = filepath
    self.triangulate = triangulate


def save_msh(settings):
  print("Saving as Mesh Geometry...")
  
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
    if obj.type == 'MESH':
      allMeshes.append(obj)
      print("Found mesh object!")
    if obj.type == 'ARMATURE':
      armature = obj
      print("Found armature object!")

  if len(allMeshes) == 0:
    print("No submeshes to export?!")
    return

  print("Mesh made from %i subMeshes" %(len(allMeshes)))
  
  allVerts    = []
  allUVs      = []
  allColours  = []

  allJointNames     = []
  allJointMatrices  = []
  allJointParents   = []
  allSkinData       = []

  vertexStarts      = []
  vertexCounts      = []
  indexStarts       = []
  indexCounts       = []
    
  indexOffset = 0
  numMeshes   = 0
 
  doColours   = False
  doUVs       = False
  doNormals   = True #Blender meshes always have them?
  doTangents  = False
  doSkins     = False
  doIndices   = True #Blender meshes always have them?

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
    
  startVert = 0
  startIndex = 0
  for mesh in allMeshes:
    if len(mesh.data.vertices) == 0:
      print("Submesh has no vertices!")
      continue
    vertCount = 0
    numMeshes+=1
    mesh_triangulate(mesh.data)
    
    meshData = MeshData()
    meshDatas.append(meshData)
	          
    uvLayer = None
    colLayer = None

    if(len(mesh.data.vertex_colors) > 0):
      doColours = True
      colLayer  = mesh.data.vertex_colors[0].data

    if(len(mesh.data.uv_layers) > 0):
      doUVs       = True
      doTangents  = True
      mesh.data.calc_tangents()
      uvLayer   = mesh.data.uv_layers.active.data

#Build up the initial set of vertex attributes - these haven't yet been indexed
    loopVerts = {l.index: l.vertex_index for l in mesh.data.loops}
    meshVerts = mesh.data.vertices
    obj_group_names = None

    if doSkins:
      obj_group_names = [g.name for g in mesh.vertex_groups]
 
    for poly in mesh.data.polygons: 
      for loop_index in poly.loop_indices:

        pos = Vector3D(meshVerts[loopVerts[loop_index]].co.x,meshVerts[loopVerts[loop_index]].co.y, meshVerts[loopVerts[loop_index]].co.z)
        allVerts.append(pos)
        vertCount = vertCount + 1

        if doUVs:
          uv = Vector2D(uvLayer[loop_index].uv[0],uvLayer[loop_index].uv[1])
          allUVs.append(uv)
        if doColours:
          colour = Vector4D(colLayer[loop_index].color[0], colLayer[loop_index].color[1],colLayer[loop_index].color[2],1.0)      
          allColours.append(colour)   

        if doSkins:
          vertex = meshVerts[loopVerts[loop_index]]
          skinData = vSkinData()
        
          for vg in vertex.groups:
            jointName = obj_group_names[vg.group]

            joint = []
            jointID = 0
            for bone in armature.pose.bones:
              if bone.name == jointName:
                joint = bone
                break
              jointID += 1

            if joint == [None]:
              print("Cant find joint %i" % jointName)
            else:
              skinData.AddData(jointID,vg.weight)
              #print("Adding weight to vertex %i" %(skinVert + v.index))
          skinData.FinaliseData()    
          allSkinData.append(skinData)
    vertexStarts.append(startVert)
    vertexCounts.append(vertCount)
    startVert = startVert + vertCount
      

  #All of the colours/uvs/and positions are now in their arrays...
  #BUT, we don't have their indices...
  #That means we have to go through the list and look for identical sets...

  outIndices  = []
  outVertices = []
  outUVs      = []
  outColours  = []
  outNormals  = []
  outTangents = []
  outSkinData = []
  #work through each submesh, build up from there

  firstOutVert = 0
  for m in range(0, len(vertexStarts)):
    firstVert = vertexStarts[m]
    lastVert = firstVert + vertexCounts[m]
    uniqueCount = 0

    for i in range(firstVert, lastVert):
      isUnique = True
      for j in range(firstOutVert, len(outVertices)):
        posCompare = allVerts[i].Compare(outVertices[j], 0.0001)
        uvCompare  = True
        colCompare = True
        if(doUVs):
          uvCompare  = allUVs[i].Compare(outUVs[j], 0.001)
        
        if(doColours):
          colCompare = allColours[i].Compare(outColours[j], 0.001)
        
        if(posCompare and uvCompare and colCompare):
          outIndices.append(j)
          isUnique = False
          break
      if isUnique:      #this is a unique vertex!
        uniqueCount = uniqueCount + 1  
        outIndices.append(len(outVertices))
        outVertices.append(allVerts[i])
        if(doUVs):
          outUVs.append(allUVs[i])
        if(doColours):
          outColours.append(allColours[i])
        if(doSkins):
          outSkinData.append(allSkinData[i])
    firstOutVert  = firstOutVert + uniqueCount
 
 #We have to now generate normals for all of our verts
  print("Original Vertex Count %i" %(len(allVerts)) )
  print("Unique Vertex Count %i" %(len(outVertices)) )
  print("Out index count %i" %(len(outIndices)) )

  if doNormals:
    for i in range(0, len(outVertices)):
      outNormals.append(Vector3D())

    index = 0
    while index < len(outIndices):
      a = outIndices[index+0]
      b = outIndices[index+1]
      c = outIndices[index+2]

      ba = Vector3D()
      ba.AddValues(outVertices[b])
      ba.SubtractValues(outVertices[a])

      ca = Vector3D()
      ca.AddValues(outVertices[c])
      ca.SubtractValues(outVertices[a])

      normal = ba.CrossProduct(ca)
      outNormals[a].AddValues(normal)
      outNormals[b].AddValues(normal)
      outNormals[c].AddValues(normal)
      index = index + 3
    for normal in outNormals:
      normal.Normalise()

  if doUVs and doTangents:
    for i in range(0, len(outVertices)):
      outTangents.append(Vector3D())
  #for tangent in allTangents:
  #  tangent.Normalise()  

  #doNormals = False
  doTangents = False
#All the arrays are filled up now, we can write them all out
  numChunks = 2 ##We always get positions and indices

  print("Output mesh has %i vertices" %(len(outVertices)))

  if doNormals:
    numChunks += 1
    print("Output mesh has %i Normals" %(len(outNormals)))

  if doSkins:
    numChunks += 5 ##Weights, indices, jointNames, bindpose, invbindpose

  if doUVs:
    numChunks += 1
    print("Output mesh has %i UVs" %(len(outUVs)))
  
  if doTangents:
    numChunks += 1
    print("Output mesh has %i Tangents" %(len(outTangents)))

  if doColours:
    numChunks += 1
    print("Output mesh has %i Colours" %(len(outColours)))

  print("Exporting %i submeshes" % numMeshes)
  out = open(settings.filepath, 'w')
  out.write('MeshGeometry\n');
  out.write('%i\n' % 1)	#version
  out.write('%i\n' % numMeshes) #num meshes 
  out.write('%i\n' % len(outVertices))#this will be num vertices
  out.write('%i\n' % len(outIndices)) #this will be num indices
  out.write('%i\n' % numChunks)  #this will be the num chunks
   
  out.write('%i\n' % VPositions)
  for vert in outVertices:
    vert.Print(out)

  if doColours:
    out.write('%i\n' % VColors)  
    for colour in outColours:
      colour.Print(out)

  if doUVs: 
    out.write('%i\n' % VTex0)
    for tex in outUVs:
      tex.Print(out) 

  out.write('%i\n' % Indices)
  i = 0
  while i < len(outIndices):
    out.write( '%i %i %i' % (outIndices[i], outIndices[i+1], outIndices[i+2] ) )
    i += 3
    out.write('\n') 
	  
  if doNormals:  
    out.write('%i\n' % VNormals)
    for normal in outNormals:
      normal.Print(out)
		  
  if doTangents:
    out.write('%i\n' % VTangents)
    for tangent in outTangents:
      tangent.Print(out)
	  
  if doSkins:
    out.write('%i\n' % VWeightValues)
    for skinData in outSkinData:
      out.write( '%f %f %f %f\n' % (skinData.weights[0], skinData.weights[1], skinData.weights[2], skinData.weights[3]) )

    out.write('%i\n' % VWeightIndices)
    for skinData in outSkinData: 
      out.write( '%i %i %i %i\n' % (skinData.indices[0], skinData.indices[1], skinData.indices[2], skinData.indices[3]) )

    out.write('%i\n' % JointNames)
    out.write('%i\n' % len(allJointNames))
    for name in allJointNames:
     out.write('%s\n' % name )
	  
    out.write('%i\n' % JointParents)
    out.write('%i\n' % len(allJointParents))
    for parent in allJointParents:
     out.write('%i\n' % parent )

    out.write('%i\n' % BindPose)
    out.write('%i\n' % len(allJointMatrices))
    for mat in allJointMatrices:
      out.write( '%f %f %f %f\n' % (mat[0][0], mat[0][1], mat[0][2],mat[0][3] ))
      out.write( '%f %f %f %f\n' % (mat[1][0], mat[1][1], mat[1][2],mat[1][3] ))
      out.write( '%f %f %f %f\n' % (mat[2][0], mat[2][1], mat[2][2],mat[2][3] ))
      out.write( '%f %f %f %f\n' % (mat[3][0], mat[3][1], mat[3][2],mat[3][3] ))
      out.write( '\n' )

    out.write('%i\n' % BindPoseInv)
    out.write('%i\n' % len(allJointMatrices))
    for mat in allJointMatrices:
      matInv = mat.inverted()
      out.write( '%f %f %f %f\n' % (matInv[0][0], matInv[0][1], matInv[0][2],matInv[0][3] ))
      out.write( '%f %f %f %f\n' % (matInv[1][0], matInv[1][1], matInv[1][2],matInv[1][3] ))
      out.write( '%f %f %f %f\n' % (matInv[2][0], matInv[2][1], matInv[2][2],matInv[2][3] ))
      out.write( '%f %f %f %f\n' % (matInv[3][0], matInv[3][1], matInv[3][2],matInv[3][3] ))
      out.write( '\n' )

  if len(allMeshes) > 1:
    out.write('%i\n' % SubMeshes)
    for i in range(0, len(allMeshes)):
      out.write('%i %i\n' % (indexStarts[i], indexCounts[i] ))
  out.close()
	
  print("Finished saving mesh geometry!")

class ExportMsh(bpy.types.Operator):
  '''Export to .msh'''
  bl_idname = "export.msh"
  bl_label = 'Export MeshGeometry'
  filename_ext = ".msh"
    
  filepath    = StringProperty(name="File Path", description="Filepath for exporting", maxlen= 1024, default="")
  triangulate = BoolProperty(name="Triangulate", description="Transform all geometry to triangles",default=True)
  
  triangulate_property = True
  
  def execute(self, context):
    settings = ExportSettings(self.properties.filepath, self.properties.triangulate)
    save_msh(settings)
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
	self.layout.operator(ExportMsh.bl_idname, text="NCL MeshGeometry", icon='EXPORT')
	
def register():
    print("Registering MeshGeometry addon")
    bpy.utils.register_class(ExportMsh)
    bpy.types.INFO_MT_file_export.append(menu_func)
	
def unregister():
    print("Unregistering MeshGeometry addon")
    bpy.utils.unregister_class(ExportMsh)
    bpy.types.INFO_MT_file_export.remove(menu_func)
	
if __name__ == "__main__":
    register()