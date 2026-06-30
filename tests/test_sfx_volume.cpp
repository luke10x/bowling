#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../3rdparty/json/tests/thirdparty/doctest/doctest.h"

#include "../../eggsfm/xfm_api.h"

#include <cstdint>
#include <cstdlib>

static xfm_patch_opn MakeSimpleLoudPatch()
{
    xfm_patch_opn patch{};
    patch.ALG = 7;
    patch.FB = 0;
    patch.AMS = 0;
    patch.FMS = 0;
    for (int op = 0; op < 4; ++op)
    {
        patch.op[op].DT = 0;
        patch.op[op].MUL = 1;
        patch.op[op].TL = 0;
        patch.op[op].RS = 0;
        patch.op[op].AR = 31;
        patch.op[op].AM = 0;
        patch.op[op].DR = 8;
        patch.op[op].SR = 0;
        patch.op[op].SL = 0;
        patch.op[op].RR = 8;
        patch.op[op].SSG = 0;
    }
    return patch;
}

static long long RenderSfxEnergy(const char *pattern)
{
    xfm_module *module = xfm_module_create(44100, 256, XFM_CHIP_YM3438);
    REQUIRE(module != nullptr);

    xfm_patch_opn patch = MakeSimpleLoudPatch();
    xfm_patch_set(module, 0, &patch, (int)sizeof(patch), XFM_CHIP_YM3438);
    REQUIRE(xfm_sfx_declare(module, 1, pattern, 60, 1) == 1);
    REQUIRE(xfm_sfx_play(module, 1, 5) != FM_VOICE_INVALID);

    int16_t buffer[4096 * 2] = {};
    xfm_mix_sfx(module, buffer, 4096);

    long long energy = 0;
    for (int i = 0; i < 4096 * 2; ++i)
        energy += std::llabs((long long)buffer[i]);

    xfm_module_destroy(module);
    return energy;
}

TEST_CASE("SFX row volume changes rendered loudness")
{
    const char *loudPattern =
        "4\n"
        "C-4007F\n"
        ".......\n"
        "REL....\n"
        ".......\n";

    const char *quietPattern =
        "4\n"
        "C-40020\n"
        ".......\n"
        "REL....\n"
        ".......\n";

    const long long loudEnergy = RenderSfxEnergy(loudPattern);
    const long long quietEnergy = RenderSfxEnergy(quietPattern);

    CHECK(loudEnergy > quietEnergy);
    CHECK(loudEnergy > quietEnergy * 2);
}
