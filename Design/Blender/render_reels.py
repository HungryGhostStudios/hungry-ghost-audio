"""Original REEL spool geometry. Blender background: --python render_reels.py [-- preview]."""
import bpy, math, os, sys
from mathutils import Vector
ROOT = os.path.dirname(os.path.abspath(__file__))
# Use the same materials, colour management and render setup as the suite metalwork.
with open(os.path.join(ROOT, 'render_assets.py')) as f:
    exec(compile(f.read().split('graphite = material(')[0], __file__, 'exec'))
scene.cycles.samples = 64
scene.render.resolution_x = scene.render.resolution_y = 384
scene.render.resolution_percentage = 100
metal = brushed('REEL / radial machined aluminium', (.46,.49,.51), .25)
rim = material('REEL / polished chamfer', (.60,.63,.66), 1, .18)
dark = material('REEL / oxide tape', (.028,.018,.012), .1, .65)
black = material('REEL / black hub recess', (.012,.014,.015), .6, .3)
bpy.ops.object.empty_add()
rotor = bpy.context.object
rotor.name = 'REEL / rotating spool assembly'
def cylinder(name, radius, depth, z, mat, verts=128, parent=True):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=(0,0,z))
    o=bpy.context.object; o.name=name; o.data.materials.append(mat)
    if parent: o.parent=rotor
    return o
def finish(o, width=.012):
    m=o.modifiers.new('Machined edge bevel', 'BEVEL'); m.width=width; m.segments=4
    m=o.modifiers.new('Weighted metal normals', 'WEIGHTED_NORMAL')
    for p in o.data.polygons:p.use_smooth=True
plate=cylinder('Three-window aluminium flange',1,.075,.04,metal)
for j in range(3):
    # True through-cut curved windows, leaving three broad structural spokes.
    outline=[]
    for radius, angles in [(.81,range(5,86,4)),(.34,range(85,4,-4))]:
        for deg in angles:
            a=math.radians(deg+j*120); outline.append((radius*math.cos(a),radius*math.sin(a)))
    n=len(outline); verts=[(x,y,z) for z in [-.2,.3] for x,y in outline]
    faces=[tuple(range(n-1,-1,-1)),tuple(range(n,n*2))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    mesh=bpy.data.meshes.new('Window tool');mesh.from_pydata(verts,[],faces);mesh.update()
    cutter=bpy.data.objects.new('Window cutter',mesh);scene.collection.objects.link(cutter)
    bpy.context.view_layer.objects.active=plate
    m=plate.modifiers.new('Through window', 'BOOLEAN');m.operation='DIFFERENCE';m.object=cutter
    bpy.ops.object.modifier_apply(modifier=m.name);bpy.data.objects.remove(cutter,do_unlink=True)
finish(plate,.015)
finish(cylinder('Tape winding behind flange',.94,.095,-.045,dark,parent=False),.008)
for radius in [.39,.45,.51,.57,.63,.69,.75,.81,.87,.93]:
    bpy.ops.mesh.primitive_torus_add(major_radius=radius,minor_radius=.003,major_segments=128,minor_segments=6,location=(0,0,.006))
    o=bpy.context.object;o.name='Visible tape winding';o.data.materials.append(dark)
finish(cylinder('Raised aluminium NAB hub',.285,.11,.125,rim),.016)
finish(cylinder('Recessed locking insert',.205,.025,.19,black),.009)
finish(cylinder('Hexagonal spindle lock',.118,.065,.212,metal,6),.012)
finish(cylinder('Centre screw',.044,.015,.252,rim),.004)
for j in range(3):
    a=math.radians(105+j*120)
    o=cylinder('Flange fastener',.031,.013,.087,black,32)
    o.location.x=.87*math.cos(a);o.location.y=.87*math.sin(a);finish(o,.003)
def light(name,loc,energy,size,size_y,color):
    bpy.ops.object.light_add(type='AREA',location=loc)
    o=bpy.context.object;o.name=name;o.data.energy=energy;o.data.shape='RECTANGLE';o.data.size=size;o.data.size_y=size_y;o.data.color=color
    o.rotation_euler=(Vector((0,0,0))-o.location).to_track_quat('-Z','Y').to_euler()
light('Suite upper-left softbox',(-2.5,3.5,4),420,2.4,.7,(.90,.95,1))
light('Suite right edge strip',(3,.8,2),160,1.8,.4,(.70,.83,.80))
light('Suite warm lower fill',(-.5,-4,2),95,2.5,.4,(1,.91,.79))
bpy.ops.object.camera_add(location=(0,0,7))
cam=bpy.context.object;cam.data.type='ORTHO';cam.data.ortho_scale=2.2;scene.camera=cam
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT,'REEL-spool.blend'))
out=os.path.join(ROOT,'reel-frames');os.makedirs(out,exist_ok=True)
count=1 if 'preview' in sys.argv else 24
for i in range(count):
    rotor.rotation_euler.z=math.radians(i*15)
    scene.render.filepath=os.path.join(out,f'reel-{i:02d}.png')
    bpy.ops.render.render(write_still=True)
