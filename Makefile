# vim: set tabstop=4 shiftwidth=4 expandtab noexpandtab:
ASSMAN ?= build/macos/bin/assman
INKSCAPE ?= /Applications/Inkscape.app/Contents/MacOS/inkscape
CXX ?= clang++

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

# Phase 3: Convert WAV files to xxd headers
sounds: run-wav-exporter convert-wavs-to-xxd

convert-wavs-to-xxd:
	mkdir -p assets/sound_out
	xxd -i -n song_01_xxd assets/sound_in/song_01.wav assets/sound_out/song_01.h
	xxd -i -n song_02_xxd assets/sound_in/song_02.wav assets/sound_out/song_02.h
	xxd -i -n song_03_xxd assets/sound_in/song_03.wav assets/sound_out/song_03.h
	xxd -i -n song_04_xxd assets/sound_in/song_04.wav assets/sound_out/song_04.h
	xxd -i -n sfx_ball_hit_lane_xxd assets/sound_in/sfx_ball_hit_lane.wav assets/sound_out/sfx_ball_hit_lane.h
	xxd -i -n sfx_ball_hit_pins_xxd assets/sound_in/sfx_ball_hit_pins.wav assets/sound_out/sfx_ball_hit_pins.h
	xxd -i -n sfx_pin_hit_pin_xxd assets/sound_in/sfx_pin_hit_pin.wav assets/sound_out/sfx_pin_hit_pin.h
	xxd -i -n sfx_score_display_xxd assets/sound_in/sfx_score_display.wav assets/sound_out/sfx_score_display.h
	xxd -i -n sfx_gutter_xxd assets/sound_in/sfx_gutter.wav assets/sound_out/sfx_gutter.h
	xxd -i -n sfx_timeout_xxd assets/sound_in/sfx_timeout.wav assets/sound_out/sfx_timeout.h

MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
dependencies:
	cmake -S $(MAKEFILE_DIR)3rdparty -B $(MAKEFILE_DIR)build

.PHONY: assets sounds wav-exporter run-wav-exporter convert-wavs-to-xxd
