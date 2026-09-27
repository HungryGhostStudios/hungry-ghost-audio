"""Hungry Ghost / Monolith. Original geometry, no external textures or add-ons.
Run: blender --background --factory-startup --python render_assets.py -- preview
Run without 'preview' to render the complete 96-frame dial and faceplate.
"""
import bpy
import math
import os
import sys
from mathutils import Vector

ROOT = os.path.dirname(os.path.abspath(__file__))
ASSETS = os.path.abspath(os.path.join(ROOT, '..', '..', 'Assets'))
FRAMES = os.path.join(ROOT, 'frames')
os.makedirs(ASSETS, exist_ok=True)
os.makedirs(FRAMES, exist_ok=True)
PREVIEW = 'preview' in sys.argv
FACEPLATE_ONLY = 'faceplate' in sys.argv

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.render.engine = 'CYCLES'
scene.cycles.samples = 32
scene.cycles.use_denoising = True
scene.cycles.adaptive_threshold = .04
prefs = bpy.context.preferences.addons['cycles'].preferences
try:
    prefs.compute_device_type = 'OPTIX'
    prefs.get_devices()
    for device in prefs.devices:
        device.use = device.type != 'CPU'
    scene.cycles.device = 'GPU'
except Exception:
    scene.cycles.device = 'CPU'
scene.render.film_transparent = True
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
scene.render.image_settings.color_depth = '8'
scene.render.resolution_percentage = 100
scene.view_settings.view_transform = 'AgX'
scene.world.color = (.055, .055, .055)

def material(name, color, metal=0, rough=.4, grain=0):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Roughness'].default_value = rough
    if grain:
        noise = nodes.new('ShaderNodeTexNoise')
        noise.inputs['Scale'].default_value = 230
        noise.inputs['Detail'].default_value = 2
        bump = nodes.new('ShaderNodeBump')
        bump.inputs['Strength'].default_value = grain
        bump.inputs['Distance'].default_value = .014
        mat.node_tree.links.new(noise.outputs['Fac'], bump.inputs['Height'])
        mat.node_tree.links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    return mat

def brushed(name, color, rough=.32):
    mat = material(name, color, .92, rough)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    coord = nodes.new('ShaderNodeTexCoord')
    stretch = nodes.new('ShaderNodeVectorMath')
    stretch.operation = 'MULTIPLY'
    stretch.inputs[1].default_value = (6, 1100, 35)
    noise = nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = 1
    noise.inputs['Detail'].default_value = 2
    bump = nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = .14
    bump.inputs['Distance'].default_value = .0015
    links.new(coord.outputs['Generated'], stretch.inputs[0])
    links.new(stretch.outputs['Vector'], noise.inputs['Vector'])
    links.new(noise.outputs['Fac'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], nodes.get('Principled BSDF').inputs['Normal'])
    return mat

graphite = material('Ceramic graphite / fine bead blast', (.016, .019, .018), .38, .48, .28)
titanium = material('Black titanium / satin machined facets', (.20, .225, .234), .95, .23)
cap_metal = brushed('Brushed gunmetal dial face', (.045, .052, .056), .30)
edge = material('Polished titanium edge', (.28, .30, .29), .95, .21)
socket_mat = material('Anodised recessed socket', (.012, .018, .017), .55, .32)
bone = material('Ivory indicator insert', (.80, .80, .70), .12, .32)

def bevel(obj, amount, segments=3):
    mod = obj.modifiers.new('Precision edge bevel', 'BEVEL')
    mod.width = amount
    mod.segments = segments
    obj.modifiers.new('Weighted face normals', 'WEIGHTED_NORMAL')

def cylinder(name, radius, depth, z, mat, vertices=128, bevel_size=.015):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=(0, 0, z))
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(mat)
    bevel(obj, bevel_size)
    for polygon in obj.data.polygons:
        polygon.use_smooth = len(polygon.vertices) == 4
    return obj

rotor = bpy.data.objects.new('Dial / 270 degree travel', None)
scene.collection.objects.link(rotor)
socket = cylinder('Fixed inset socket', 1.09, .08, .035, socket_mat)
# Sloped polygonal crown: each plane reflects the fixed softboxes differently.
bands = [(.89, .055), (.98, .10), (.975, .18), (.86, .27)]
vertices = [(r * math.cos(i * math.tau / 16), r * math.sin(i * math.tau / 16), z)
            for r, z in bands for i in range(16)]
faces = [(j * 16 + i, j * 16 + (i + 1) % 16,
          (j + 1) * 16 + (i + 1) % 16, (j + 1) * 16 + i)
         for j in range(3) for i in range(16)]
mesh = bpy.data.meshes.new('Sculpted crown mesh')
mesh.from_pydata(vertices, [], faces)
mesh.materials.append(titanium)
ring = bpy.data.objects.new('Sixteen sculpted grip facets', mesh)
scene.collection.objects.link(ring)
bevel(ring, .014, 3)
ring.parent = rotor
rim = cylinder('Fine highlight lip', .853, .035, .260, edge, 128, .009)
rim.parent = rotor
cap = cylinder('Brushed metal face', .825, .045, .287, cap_metal, 128, .012)
cap.parent = rotor
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, .682, .318))
indicator = bpy.context.object
indicator.name = 'Inlaid ivory position marker'
indicator.dimensions = (.034, .185, .014)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
indicator.data.materials.append(bone)
bevel(indicator, .008, 4)
indicator.parent = rotor

def area(name, location, energy, size, color, size_y=None):
    light = bpy.data.lights.new(name, 'AREA')
    light.energy = energy
    light.color = color
    light.shape = 'RECTANGLE'
    light.size = size
    light.size_y = size_y or size
    obj = bpy.data.objects.new(name, light)
    scene.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector((0, 0, .1)) - obj.location).to_track_quat('-Z', 'Y').to_euler()
    return obj

area('Long softbox / upper left', (-2.5, 3.5, 4), 420, 2.4, (.90, .95, 1), .7)
area('Narrow rim / right', (3, .8, 2), 160, 1.8, (.70, .83, .80), .4)
area('Low fill / lower edge', (-.5, -4, 2), 95, 2.5, (1, .91, .79), .4)
bpy.ops.object.camera_add(location=(0, 0, 7))
camera = bpy.context.object
camera.name = 'Orthographic UI asset camera'
camera.data.type = 'ORTHO'
camera.data.ortho_scale = 2.60
camera.rotation_euler = (0, 0, 0)
scene.camera = camera
scene.render.resolution_x = 256
scene.render.resolution_y = 256
rotor.rotation_euler.z = -math.radians(360)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, 'Monolith-dial.blend'))

for index in ([] if FACEPLATE_ONLY else ([47] if PREVIEW else range(96))):
    rotor.rotation_euler.z = -math.radians(225 + index * 270 / 95)
    scene.render.filepath = os.path.join(FRAMES, f'dial-{index:03d}.png')
    bpy.ops.render.render(write_still=True)
    print(f'DIAL {index + 1}/96', flush=True)

if not PREVIEW:
    for obj in [socket, ring, rim, cap, indicator, rotor]:
        obj.hide_render = True
    bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, .03))
    slab = bpy.context.object
    slab.name = 'Monolith / single machined chassis'
    slab.dimensions = (2.52, 1.858, .16)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    chassis_mat = brushed('Brushed gunmetal chassis / horizontal machining', (.032, .038, .042), .36)
    slab.data.materials.append(chassis_mat)
    bevel(slab, .026, 6)
    camera.data.ortho_scale = 2.60
    scene.render.resolution_x = 1840
    scene.render.resolution_y = 1360
    scene.cycles.samples = 64
    scene.render.filepath = os.path.join(ASSETS, 'monolith-faceplate.png')
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, 'Monolith-faceplate.blend'))
    bpy.ops.render.render(write_still=True)
    print('FACEPLATE COMPLETE', flush=True)
    slab.hide_render = True

    def block(name, dimensions, location, mat, edge_size):
        bpy.ops.mesh.primitive_cube_add(size=1, location=location)
        obj = bpy.context.object
        obj.name = name
        obj.dimensions = dimensions
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        obj.data.materials.append(mat)
        bevel(obj, edge_size, 5)
        return obj

    # A real metal lip surrounding an inset smoked display, under the same lights.
    rim_mat = brushed('Milled display lip', (.055, .062, .065), .28)
    glass_mat = material('Smoked instrument glass', (.003, .009, .007), .45, .18)
    frame = block('Display pocket / machined lip', (2.52, .885, .10), (0, 0, .03), rim_mat, .023)
    glass = block('Recessed smoked glass', (2.43, .798, .026), (0, 0, .073), glass_mat, .010)
    scene.render.resolution_x = 1248
    scene.render.resolution_y = 438
    scene.render.filepath = os.path.join(ASSETS, 'monolith-display.png')
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, 'Monolith-display.blend'))
    bpy.ops.render.render(write_still=True)
    frame.hide_render = True
    glass.hide_render = True

    # Replaceable physical keycap used by the suite's selectors.
    base = block('Selector key socket', (2.52, 1.08, .07), (0, 0, .01), socket_mat, .035)
    key = block('Selector metal keycap', (2.43, .986, .05), (0, 0, .052), cap_metal, .027)
    scene.render.resolution_x = 304
    scene.render.resolution_y = 136
    scene.render.filepath = os.path.join(ASSETS, 'monolith-button.png')
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, 'Monolith-button.blend'))
    bpy.ops.render.render(write_still=True)
    jade = material('Dim jade selector illumination', (.15, .35, .27), .35, .3)
    bsdf = jade.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Emission Color'].default_value = (.12, .42, .30, 1)
    bsdf.inputs['Emission Strength'].default_value = .45
    for dims, loc in [((2.33, .014, .014), (0, .466, .083)),
                      ((2.33, .014, .014), (0, -.466, .083)),
                      ((.014, .924, .014), (-1.165, 0, .083)),
                      ((.014, .924, .014), (1.165, 0, .083))]:
        block('Inset jade perimeter', dims, loc, jade, .006)
    scene.render.filepath = os.path.join(ASSETS, 'monolith-button-active.png')
    bpy.ops.render.render(write_still=True)
    print('DISPLAY AND SELECTOR COMPLETE', flush=True)
