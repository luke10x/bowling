#include "framework/boot.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_truetype.h>
#undef STB_IMAGE_IMPLEMENTATION
#undef STB_TRUETYPE_IMPLEMENTATION

int main(int argc, char *argv[]) { vtx::openVortex(480, 854); }

void vtx::exitVortex(int i) {}