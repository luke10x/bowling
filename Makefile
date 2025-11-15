# vim: set tabstop=4 shiftwidth=4 expandtab noexpandtab:
ASSMAN ?= build/macos/bin/assman
assets:
	blender -b assets/artwork/bowling.blend --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='./assets/assman_in/bowling.glb', export_yup=1)"
	mkdir -p assets/assman_in
	mkdir -p assets/assman_out
	$(ASSMAN) mesh assets/assman_in/bowling.glb pinMesh \
		-o assets/assman_out/pin.mesh

.PHONY: assets
