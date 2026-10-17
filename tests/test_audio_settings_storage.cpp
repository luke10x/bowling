#include "../storage.h"

#include <cassert>
#include <cstring>
#include <unistd.h>

int main()
{
    char path[] = "/tmp/bowling-audio-settings-XXXXXX";
    const int fd = mkstemp(path);
    assert(fd >= 0);
    close(fd);

    Storage storage{};
    std::strncpy(storage.filePath, path, sizeof(storage.filePath) - 1);
    assert(storage.reset() == Storage::KEY_COUNT);

    char value[32] = {};
    assert(storage.getChar(Storage::AUDIO_BUFFER_SIZE, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "2048") == 0);
    assert(storage.getChar(Storage::AUDIO_BUFFER_MANUAL, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "0") == 0);
    assert(storage.getChar(Storage::HIGH_DETAILS, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "1") == 0);
    assert(storage.getChar(Storage::PATCHED_YMFM, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "0") == 0);

    storage.setChar(Storage::AUDIO_BUFFER_SIZE, "8192", 4);
    storage.setChar(Storage::AUDIO_BUFFER_MANUAL, "1", 1);
    storage.setChar(Storage::HIGH_DETAILS, "0", 1);
    storage.setChar(Storage::PATCHED_YMFM, "1", 1);
    assert(storage.getChar(Storage::AUDIO_BUFFER_SIZE, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "8192") == 0);
    assert(storage.getChar(Storage::HIGH_DETAILS, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "0") == 0);
    assert(storage.getChar(Storage::PATCHED_YMFM, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "1") == 0);

    assert(storage.reset() == Storage::KEY_COUNT);
    assert(storage.getChar(Storage::AUDIO_BUFFER_SIZE, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "2048") == 0);
    assert(storage.getChar(Storage::AUDIO_BUFFER_MANUAL, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "0") == 0);
    assert(storage.getChar(Storage::HIGH_DETAILS, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "1") == 0);
    assert(storage.getChar(Storage::PATCHED_YMFM, value, sizeof(value)) > 0);
    assert(std::strcmp(value, "0") == 0);
    unlink(path);
}
