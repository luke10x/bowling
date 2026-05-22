# vim: set tabstop=4 shiftwidth=4 expandtab noexpandtab:
ASSMAN ?= build/macos/bin/assman
INKSCAPE ?= /Applications/Inkscape.app/Contents/MacOS/inkscape
CXX ?= clang++

assets:
	mkdir -p assets/assman_in
	mkdir -p assets/assman_out
	blender -b assets/artwork/bowling.blend --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='./assets/assman_in/bowling.glb', export_yup=1)"
	blender -b assets/artwork/angel.blend --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='./assets/assman_in/angel.glb', export_yup=1)"
	blender -b assets/artwork/cherub.blend --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='./assets/assman_in/cherub.glb', export_yup=1)"
	blender -b assets/artwork/seraph.blend --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='./assets/assman_in/seraph.glb', export_yup=1)"
	blender -b assets/artwork/throne.blend --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='./assets/assman_in/throne.glb', export_yup=1)"
	$(ASSMAN) mesh assets/assman_in/bowling.glb ballMesh \
		-o assets/assman_out/ball.mesh
	$(ASSMAN) mesh assets/assman_in/bowling.glb laneMesh \
		-o assets/assman_out/lane.mesh
	$(ASSMAN) mesh assets/assman_in/bowling.glb pinMesh \
		-o assets/assman_out/pin.mesh
	$(ASSMAN) mesh assets/assman_in/bowling.glb StarPillMesh \
		-o assets/assman_out/star.mesh
	$(ASSMAN) mesh assets/assman_in/angel.glb AngelMesh \
		-o assets/assman_out/angel.mesh
	$(ASSMAN) animation assets/assman_in/angel.glb \
		-cfg assets/assman_angel.conf \
		-o assets/assman_out/angel.anim
	@set -e; \
	if $(ASSMAN) mesh assets/assman_in/cherub.glb CherubMesh -o assets/assman_out/cherub.mesh; then \
		echo "mesh CherubMesh" > assets/assman_cherub.conf; \
	else \
		$(ASSMAN) mesh assets/assman_in/cherub.glb CherubMesh.001 -o assets/assman_out/cherub.mesh; \
		echo "mesh CherubMesh.001" > assets/assman_cherub.conf; \
	fi; \
	echo "clip BowlingThrow" >> assets/assman_cherub.conf; \
	echo "clip BowlingArgument" >> assets/assman_cherub.conf; \
	$(ASSMAN) animation assets/assman_in/cherub.glb -cfg assets/assman_cherub.conf -o assets/assman_out/cherub.anim
	$(ASSMAN) mesh assets/assman_in/seraph.glb SeraphMesh \
		-o assets/assman_out/seraph.mesh
	@set -e; \
	echo "mesh SeraphMesh" > assets/assman_seraph.conf; \
	echo "clip BowlingThrow" >> assets/assman_seraph.conf; \
	echo "clip BowlingArgument" >> assets/assman_seraph.conf; \
	$(ASSMAN) animation assets/assman_in/seraph.glb -cfg assets/assman_seraph.conf -o assets/assman_out/seraph.anim
	$(ASSMAN) mesh assets/assman_in/throne.glb ThroneMesh \
		-o assets/assman_out/throne.mesh
	@set -e; \
	echo "mesh ThroneMesh" > assets/assman_throne.conf; \
	echo "clip BowlingThrow" >> assets/assman_throne.conf; \
	echo "clip BowlingArgument" >> assets/assman_throne.conf; \
	$(ASSMAN) animation assets/assman_in/throne.glb -cfg assets/assman_throne.conf -o assets/assman_out/throne.anim
	xxd -i -n ball_mesh_data \
	 	assets/assman_out/ball.mesh \
		assets/xxd_mesh/ball_mesh.h
	xxd -i -n pin_mesh_data \
	 	assets/assman_out/pin.mesh \
		assets/xxd_mesh/pin_mesh.h
	xxd -i -n lane_mesh_data \
	 	assets/assman_out/lane.mesh \
		assets/xxd_mesh/lane_mesh.h
	xxd -i -n star_mesh_data \
	 	assets/assman_out/star.mesh \
		assets/xxd_mesh/star_mesh.h
	xxd -i -n angel_mesh_data \
	 	assets/assman_out/angel.mesh \
		assets/xxd_mesh/angel_mesh.h
	xxd -i -n angel_anim_data \
	 	assets/assman_out/angel.anim \
		assets/xxd_mesh/angel_anim.h
	xxd -i -n cherub_mesh_data \
	 	assets/assman_out/cherub.mesh \
		assets/xxd_mesh/cherub_mesh.h
	xxd -i -n cherub_anim_data \
	 	assets/assman_out/cherub.anim \
		assets/xxd_mesh/cherub_anim.h
	xxd -i -n seraph_mesh_data \
	 	assets/assman_out/seraph.mesh \
		assets/xxd_mesh/seraph_mesh.h
	xxd -i -n seraph_anim_data \
	 	assets/assman_out/seraph.anim \
		assets/xxd_mesh/seraph_anim.h
	xxd -i -n throne_mesh_data \
	 	assets/assman_out/throne.mesh \
		assets/xxd_mesh/throne_mesh.h
	xxd -i -n throne_anim_data \
	 	assets/assman_out/throne.anim \
		assets/xxd_mesh/throne_anim.h
	$(INKSCAPE) assets/artwork/everything_tex.svg \
		--export-id=exportroot \
		--export-id-only \
		--export-area-page \
		--export-type=png \
		--export-filename="assets/files/everything_tex.png"

# Phase 2: Build and run WAV exporter
wav-exporter: build/macos/bin/game-wav-exporter

CXXFLAGS += -I../my-ym2612-plugin/build/_deps/ymfm-src/src

LDLIBS += -L./build/macos/usr/lib
LDLIBS += $(PWD)/../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_misc.cpp
LDLIBS += $(PWD)/../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_adpcm.cpp
LDLIBS += $(PWD)/../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_ssg.cpp
LDLIBS += $(PWD)/../my-ym2612-plugin/build/_deps/ymfm-src/src/ymfm_opn.cpp
build/macos/bin/game-wav-exporter: game-wav-exporter.cpp sounds/songs_data.h
	mkdir -p build/macos/bin
	$(CXX) -std=c++20 -O2 -I. -I./3rdparty/glm -I./3rdparty/SDL/include \
		$(CXXFLAGS) \
		game-wav-exporter.cpp \
		$(LDLIBS) \
		-o build/macos/bin/game-wav-exporter
# 		-lSDL2

run-wav-exporter: wav-exporter
	mkdir -p assets/sound_in
	./build/macos/bin/game-wav-exporter

# Phase 3: Convert WAV files to cpp files and create header
sounds: run-wav-exporter convert-wavs-to-xxd

convert-wavs-to-xxd:
	mkdir -p assets/sound_out
	xxd -i -n song_01_xxd assets/sound_in/song_01.wav > assets/sound_out/song_01_xxd.cpp
	xxd -i -n song_02_xxd assets/sound_in/song_02.wav > assets/sound_out/song_02_xxd.cpp
	xxd -i -n song_03_xxd assets/sound_in/song_03.wav > assets/sound_out/song_03_xxd.cpp
	xxd -i -n song_04_xxd assets/sound_in/song_04.wav > assets/sound_out/song_04_xxd.cpp
	xxd -i -n sfx_ball_hit_lane_xxd assets/sound_in/sfx_ball_hit_lane.wav > assets/sound_out/sfx_ball_hit_lane_xxd.cpp
	xxd -i -n sfx_ball_hit_pins_xxd assets/sound_in/sfx_ball_hit_pins.wav > assets/sound_out/sfx_ball_hit_pins_xxd.cpp
	xxd -i -n sfx_pin_hit_pin_xxd assets/sound_in/sfx_pin_hit_pin.wav > assets/sound_out/sfx_pin_hit_pin_xxd.cpp
	xxd -i -n sfx_score_display_xxd assets/sound_in/sfx_score_display.wav > assets/sound_out/sfx_score_display_xxd.cpp
	xxd -i -n sfx_gutter_xxd assets/sound_in/sfx_gutter.wav > assets/sound_out/sfx_gutter_xxd.cpp
	xxd -i -n sfx_timeout_xxd assets/sound_in/sfx_timeout.wav > assets/sound_out/sfx_timeout_xxd.cpp
	# Create combined header file
	echo "#pragma once" > assets/sound_out/all_wav_xxd.h
	echo "// Auto-generated WAV data declarations" >> assets/sound_out/all_wav_xxd.h
	echo "" >> assets/sound_out/all_wav_xxd.h
	for f in assets/sound_out/*_xxd.cpp; do \
		name=$$(basename "$$f" _xxd.cpp); \
		echo "extern const unsigned char $${name}_xxd[];" >> assets/sound_out/all_wav_xxd.h; \
		echo "extern unsigned int $${name}_xxd_len;" >> assets/sound_out/all_wav_xxd.h; \
	done
	echo "" >> assets/sound_out/all_wav_xxd.h

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
dependencies:
	cmake -S $(MAKEFILE_DIR)3rdparty -B $(MAKEFILE_DIR)build

.PHONY: assets sounds wav-exporter run-wav-exporter convert-wavs-to-xxd
