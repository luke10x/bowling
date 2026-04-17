Asset pipeline
==============

There are several different types of assets.

Meshes and Animations
---------------------

Meshes and Animations are imported from GLB files using
**ASSMAN** tool. A

bundle is a directory of selectec artwork which will be certainly used


blender -b assets/artwork/humans.blend --python-expr \
    "import bpy; bpy.ops.export_scene.gltf(filepath='./assets/assman_data/humans.glb', export_yup=1)"
assman --file assets/assman_in/humanoids.glb --mesh TeacherMesh > assets/assman_out/teacher.mesh
assman --file assets/assman_in/humanoids.glb --mesh StudentMesh > assets/assman_out/student.mesh
xxd -i assets/assman_out/teacher.mesh > assets/xxd_out/teacher_blob.c
xxd -i mesh/assman_out/teacher.mesh > assets/xxd_out/teacher_blob.c
make $(all from assets/xx/*.c) combined_blob.a
make runtime.so: combined_blob.a OR make main for static exec

Scenes
------

Scenes are special blend files that describe locations and dimensions of the objects.
Inside scene "representative objects" can be used. 
Those objects will have materials assinged to them to make them of distinct color in
blender, and textures can have attributes, as well as object directly can have properties

scenes are extracted from blender files using 

    scene2cpp --input artwork/scene.blend > assets/ass_out/scene.cpp

Atlas and Images
----------------

Atlas will be loaded from svg using inkscape CLI

inkscape input.svg --export-type=png --export-filename=output.png --export-area-drawing

it should also generate C++ code from svg element classes.

Fonts
-----

Uses assman tool
TBA after investigating how to use font library



