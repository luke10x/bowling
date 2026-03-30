// -----------------------------------------------------------------------------
// game-wav-exporter - Export bowling game sounds as WAV files
//
// This program renders all songs and SFX from the game and exports them
// as WAV files to assets/sound_in/
//
// Usage: ./game-wav-exporter
// Output: assets/sound_in/song_01.wav, song_02.wav, etc.
//         assets/sound_in/sfx_*.wav
// -----------------------------------------------------------------------------

#include <SDL.h>
#include <SDL_audio.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>

#include "./../eggsfm/xfm_api.h"
#include "./../eggsfm/xfm_impl.cpp"
#include "./sounds/songs_data.h"

// Simple WAV file writer
bool write_wav_file(const char* filename, int16_t* samples, int num_samples, int sample_rate)
{
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("Error: Cannot open %s for writing\n", filename);
        return false;
    }

    // WAV file header
    uint32_t file_size = 36 + num_samples * 2;  // 16-bit stereo
    uint32_t data_size = num_samples * 2;
    uint16_t audio_format = 1;  // PCM
    uint16_t num_channels = 2;  // Stereo
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
    uint16_t block_align = num_channels * bits_per_sample / 8;

    // RIFF header
    fwrite("RIFF", 1, 4, f);
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    // fmt chunk
    fwrite("fmt ", 1, 4, f);
    uint32_t fmt_chunk_size = 16;
    fwrite(&fmt_chunk_size, 4, 1, f);
    fwrite(&audio_format, 2, 1, f);
    fwrite(&num_channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);

    // data chunk
    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
    fwrite(samples, 2, num_samples, f);

    fclose(f);
    printf("  Written: %s (%d samples, %d Hz)\n", filename, num_samples, sample_rate);
    return true;
}

// Render a song pattern to audio buffer
std::vector<int16_t> render_song(const char* song_pattern, int sample_rate, int buffer_size, int ticks_per_step)
{
    xfm_module* module = xfm_module_create(sample_rate, buffer_size, XFM_CHIP_YM3438);
    if (!module) {
        printf("Error: Failed to create xfm module\n");
        return {};
    }

    // DUBLICATED in sounds.h !!

    // Load patches: song 1
    xfm_patch_set(module, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);

    // Song 2
    xfm_patch_set(module, 0x03, &PATCH_03_GUITAR, sizeof(PATCH_03_GUITAR), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x04, &PATCH_04_SAW, sizeof(PATCH_04_SAW), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x05, &PATCH_05_FLUTE, sizeof(PATCH_05_FLUTE), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x06, &PATCH_06_FOOTBALL_KICK, sizeof(PATCH_06_FOOTBALL_KICK), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x07, &PATCH_07_SNARE, sizeof(PATCH_07_SNARE), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x08, &PATCH_08_HIHAT, sizeof(PATCH_08_HIHAT), XFM_CHIP_YM3438);

    // Declare and play song
    xfm_song_declare(module, 1, song_pattern, 60, ticks_per_step);
    xfm_song_play(module, 1, true);

    // Calculate how many frames we need (render full song - estimate 4 minutes max)
    int max_frames = sample_rate * 60 * 4;  // 4 minutes of stereo samples
    std::vector<int16_t> audio_buffer(max_frames * 2);
    int total_frames = 0;

    // Render until song stops or we hit max
    bool song_playing = true;
    while (song_playing && total_frames < max_frames) {
        int frames_to_render = std::min(buffer_size, max_frames - total_frames);
        xfm_mix_song(module, audio_buffer.data() + total_frames * 2, frames_to_render);
        total_frames += frames_to_render;

        // Check if song is still playing (simple heuristic - check if we've rendered enough)
        // In practice, xfm will output silence when done
        if (total_frames >= sample_rate * 180) {  // Assume max 3 minutes per song
            song_playing = false;
        }
    }

    // Trim trailing silence (keep last 2 seconds of non-silent audio)
    int last_non_silent = total_frames - 1;
    while (last_non_silent > 0) {
        int idx = last_non_silent * 2;
        if (audio_buffer[idx] > 100 || audio_buffer[idx] < -100 ||
            audio_buffer[idx + 1] > 100 || audio_buffer[idx + 1] < -100) {
            break;
        }
        last_non_silent--;
    }
    total_frames = last_non_silent + 1;

    // Add 2 seconds of trailing silence
    int final_size = (total_frames + sample_rate * 2) * 2;
    audio_buffer.resize(final_size);

    xfm_module_destroy(module);
    return audio_buffer;
}

// Render an SFX pattern to audio buffer
std::vector<int16_t> render_sfx(const char* sfx_pattern, int sample_rate, int buffer_size)
{
    xfm_module* module = xfm_module_create(sample_rate, buffer_size, XFM_CHIP_YM3438);
    if (!module) {
        printf("Error: Failed to create xfm module\n");
        return {};
    }

    // Load patches
    xfm_patch_set(module, 0x00, &PATCH_00_RUBBER_BASS, sizeof(PATCH_00_RUBBER_BASS), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x01, &PATCH_01_HOLLOW_ELECTRIC, sizeof(PATCH_01_HOLLOW_ELECTRIC), XFM_CHIP_YM3438);
    xfm_patch_set(module, 0x02, &PATCH_02_ANGRY_HIHAT, sizeof(PATCH_02_ANGRY_HIHAT), XFM_CHIP_YM3438);

    // Declare and play SFX
    xfm_sfx_declare(module, 0, sfx_pattern, 60, 3);
    xfm_sfx_play(module, 0, 1);

    // Render SFX (usually very short - 1 second max)
    int max_frames = sample_rate;  // 1 second
    std::vector<int16_t> audio_buffer(max_frames * 2);

    int frames_rendered = 0;
    while (frames_rendered < max_frames) {
        int frames_to_render = std::min(buffer_size, max_frames - frames_rendered);
        xfm_mix_sfx(module, audio_buffer.data() + frames_rendered * 2, frames_to_render);
        frames_rendered += frames_to_render;
    }

    xfm_module_destroy(module);
    return audio_buffer;
}

int main(int argc, char* argv[])
{
    printf("=== Bowling Game WAV Exporter ===\n\n");

    // const int sample_rate = 44100;
    const int sample_rate = 11025;
    const int buffer_size = 256;

    // Export songs
    printf("Exporting songs:\n");

    auto song1_audio = render_song(SONG_01, sample_rate, buffer_size, 6);
    if (!song1_audio.empty()) {
        write_wav_file("assets/sound_in/song_01.wav", song1_audio.data(), song1_audio.size(), sample_rate);
    }

    auto song2_audio = render_song(SONG_02, sample_rate, buffer_size, 8);
    if (!song2_audio.empty()) {
        write_wav_file("assets/sound_in/song_02.wav", song2_audio.data(), song2_audio.size(), sample_rate);
    }

    auto song3_audio = render_song(SONG_03, sample_rate, buffer_size, 6);
    if (!song3_audio.empty()) {
        write_wav_file("assets/sound_in/song_03.wav", song3_audio.data(), song3_audio.size(), sample_rate);
    }

    auto song4_audio = render_song(SONG_04, sample_rate, buffer_size, 6);
    if (!song4_audio.empty()) {
        write_wav_file("assets/sound_in/song_04.wav", song4_audio.data(), song4_audio.size(), sample_rate);
    }

    // Export SFX
    printf("\nExporting SFX:\n");

    auto sfx1_audio = render_sfx(SFX_PAT_BALL_HIT_LANE, sample_rate, buffer_size);
    if (!sfx1_audio.empty()) {
        write_wav_file("assets/sound_in/sfx_ball_hit_lane.wav", sfx1_audio.data(), sfx1_audio.size(), sample_rate);
    }

    auto sfx2_audio = render_sfx(SFX_PAT_BALL_HIT_PINS, sample_rate, buffer_size);
    if (!sfx2_audio.empty()) {
        write_wav_file("assets/sound_in/sfx_ball_hit_pins.wav", sfx2_audio.data(), sfx2_audio.size(), sample_rate);
    }

    auto sfx3_audio = render_sfx(SFX_PAT_PIN_HIT_PIN, sample_rate, buffer_size);
    if (!sfx3_audio.empty()) {
        write_wav_file("assets/sound_in/sfx_pin_hit_pin.wav", sfx3_audio.data(), sfx3_audio.size(), sample_rate);
    }

    auto sfx4_audio = render_sfx(SFX_PAT_SCORE_DISPLAY, sample_rate, buffer_size);
    if (!sfx4_audio.empty()) {
        write_wav_file("assets/sound_in/sfx_score_display.wav", sfx4_audio.data(), sfx4_audio.size(), sample_rate);
    }

    auto sfx5_audio = render_sfx(SFX_PAT_GUTTER, sample_rate, buffer_size);
    if (!sfx5_audio.empty()) {
        write_wav_file("assets/sound_in/sfx_gutter.wav", sfx5_audio.data(), sfx5_audio.size(), sample_rate);
    }

    auto sfx6_audio = render_sfx(SFX_PAT_TIMEOUT, sample_rate, buffer_size);
    if (!sfx6_audio.empty()) {
        write_wav_file("assets/sound_in/sfx_timeout.wav", sfx6_audio.data(), sfx6_audio.size(), sample_rate);
    }

    printf("\n=== Export complete ===\n");
    return 0;
}
