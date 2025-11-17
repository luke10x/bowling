# vim: set tabstop=4 shiftwidth=4 expandtab noexpandtab:
ASSMAN ?= build/macos/bin/assman
INKSCAPE ?= /Applications/Inkscape.app/Contents/MacOS/inkscape
assets:
	blender -b assets/artwork/bowling.blend --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='./assets/assman_in/bowling.glb', export_yup=1)"
	mkdir -p assets/assman_in
	mkdir -p assets/assman_out
	$(ASSMAN) mesh assets/assman_in/bowling.glb ballMesh \
		-o assets/assman_out/ball.mesh
	$(ASSMAN) mesh assets/assman_in/bowling.glb laneMesh \
		-o assets/assman_out/lane.mesh
	$(ASSMAN) mesh assets/assman_in/bowling.glb pinMesh \
		-o assets/assman_out/pin.mesh
	xxd -i -n ball_mesh_data \
	 	assets/assman_out/ball.mesh \
		assets/xxd_mesh/ball_mesh.h
	xxd -i -n pin_mesh_data \
	 	assets/assman_out/pin.mesh \
		assets/xxd_mesh/pin_mesh.h
	xxd -i -n lane_mesh_data \
	 	assets/assman_out/lane.mesh \
		assets/xxd_mesh/lane_mesh.h
	$(INKSCAPE) assets/artwork/everything_tex.svg \
		--export-id=exportroot \
		--export-id-only \
		--export-area-page \
		--export-type=png \
		--export-filename="assets/files/everything_tex.png"

.PHONY: assets
