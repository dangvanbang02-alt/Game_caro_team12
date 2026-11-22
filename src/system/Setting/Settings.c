#include "Settings.h"
#include <stdio.h>
#ifdef _WIN32
    #include <direct.h>
    #define SETTINGS_MKDIR(path) _mkdir(path)
const char *keyActionName(KeyAction action)
{
    static const char *names[ACT_COUNT] = {
        "Len", "Xuong", "Trai", "Phai",
        "Dat quan", "Undo", "Tam dung", "Luu game"
    };

    return (action >= 0 && action < ACT_COUNT) ? names[action] : "?";
}

SettingsResult saveSettings(const Settings *settings)
{
    FILE *file;
    int i;
    int error;

    if (!validateSettings(settings)) return SETTINGS_ERR_DATA;

    (void)SETTINGS_MKDIR("saves");
    file = settings_open(SETTINGS_FILE, "w");
    if (file == NULL) return SETTINGS_ERR_OPEN;

    fprintf(file, "%s %d\n", SETTINGS_MAGIC, SETTINGS_VER);

    for (i = 0; i < ACT_COUNT; ++i) {
        fprintf(file, "%d ", settings->keys[i]);
    }

    fprintf(file, "\n%d %d %d %d\n",
            settings->musicOn, settings->musicVolume,
            settings->soundOn, settings->soundVolume);

    fprintf(file, "%d %d\n",
            settings->turnTimeLimit, settings->gameTimeLimit);
    fprintf(file, "END\n");

    error = ferror(file);
    fclose(file);

    return error ? SETTINGS_ERR_OPEN : SETTINGS_OK;
}

SettingsResult loadSettings(Settings *settings)
{
    FILE *file;
    Settings temp;
    char magic[32];
    char endTag[8];
    int version;
    int i;

    if (settings == NULL) return SETTINGS_ERR_DATA;

    file = settings_open(SETTINGS_FILE, "r");
    if (file == NULL) {
        initSettings(settings);
        return SETTINGS_ERR_NOT_FOUND;
    }

    memset(&temp, 0, sizeof(temp));

    if (fscanf(file, "%31s %d", magic, &version) != 2 ||
        strcmp(magic, SETTINGS_MAGIC) != 0) {
        fclose(file);
        return SETTINGS_ERR_FORMAT;
    }

    if (version != SETTINGS_VER) {
        fclose(file);
        return SETTINGS_ERR_VERSION;
    }

    for (i = 0; i < ACT_COUNT; ++i) {
        if (fscanf(file, "%d", &temp.keys[i]) != 1) {
            fclose(file);
            return SETTINGS_ERR_FORMAT;
        }
    }

    if (fscanf(file, "%d %d %d %d",
               &temp.musicOn, &temp.musicVolume,
               &temp.soundOn, &temp.soundVolume) != 4 ||
        fscanf(file, "%d %d",
               &temp.turnTimeLimit, &temp.gameTimeLimit) != 2) {
        fclose(file);
        return SETTINGS_ERR_FORMAT;
    }

    if (fscanf(file, "%7s", endTag) != 1 || strcmp(endTag, "END") != 0) {
        fclose(file);
        return SETTINGS_ERR_FORMAT;
    }

    fclose(file);

    if (!validateSettings(&temp)) return SETTINGS_ERR_DATA;

    *settings = temp;
    return SETTINGS_OK;
}
const char *settingsResultMessage(SettingsResult result)
{
    switch (result) {
        case SETTINGS_OK:                return "Thanh cong";
        case SETTINGS_ERR_OPEN:          return "Khong mo/ghi duoc file";
        case SETTINGS_ERR_FORMAT:        return "File sai dinh dang";
        case SETTINGS_ERR_VERSION:       return "File settings khac phien ban";
        case SETTINGS_ERR_DATA:          return "Du lieu khong hop le";
        case SETTINGS_ERR_NOT_FOUND:     return "Khong tim thay file settings";
        case SETTINGS_ERR_KEY_CONFLICT:  return "Phim da duoc gan cho hanh dong khac";
        default:                         return "Loi khong xac dinh";
    }
}
#endif