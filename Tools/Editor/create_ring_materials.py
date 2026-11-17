"""Optional visual upgrade for THE RING demo. NOT RUN by its author: unverified.

Creates two small materials the demo looks for at startup:

  /Game/TheRing/Materials/M_RingSurface   lit: Color, Roughness, Metallic parameters
  /Game/TheRing/Materials/M_RingEmissive  unlit: Color * Strength

Without them the demo still runs on the engine's BasicShapeMaterial, but every
surface has the same roughness (no glossy chamber floor) and nothing glows
(Voraik's face line, the drone indicator, the lamp lens, the stars).

Run once in the editor: Tools > Execute Python Script... and pick this file,
or in the Output Log's Python console:  py "<path>/create_ring_materials.py"
Then press Play again. Safe to run twice: existing assets are left alone.
"""

import unreal

FOLDER = "/Game/TheRing/Materials"
LIB = unreal.MaterialEditingLibrary


def _scalar(material, name, default, y):
    node = LIB.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -420, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def _vector(material, name, default, y):
    node = LIB.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -420, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def _create(name):
    path = "{}/{}".format(FOLDER, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("{} already exists; left as it is.".format(path))
        return None
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    return tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


def make_surface():
    material = _create("M_RingSurface")
    if not material:
        return
    color = _vector(material, "Color", unreal.LinearColor(0.05, 0.05, 0.055, 1.0), 0)
    roughness = _scalar(material, "Roughness", 0.6, 220)
    metallic = _scalar(material, "Metallic", 0.2, 320)
    LIB.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    LIB.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    LIB.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    LIB.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("Created M_RingSurface")


def make_emissive():
    material = _create("M_RingEmissive")
    if not material:
        return
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = _vector(material, "Color", unreal.LinearColor(1.0, 1.0, 1.0, 1.0), 0)
    strength = _scalar(material, "Strength", 4.0, 220)
    multiply = LIB.create_material_expression(material, unreal.MaterialExpressionMultiply, -180, 80)
    LIB.connect_material_expressions(color, "", multiply, "A")
    LIB.connect_material_expressions(strength, "", multiply, "B")
    LIB.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    LIB.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("Created M_RingEmissive")


make_surface()
make_emissive()
