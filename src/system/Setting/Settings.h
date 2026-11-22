#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_FILE  "saves/settings.cfg"
#define SETTINGS_MAGIC "CARO_SETTINGS"
#define SETTINGS_VER   1

typedef enum {
    ACT_UP = 0,
    ACT_DOWN,
    ACT_LEFT,
    ACT_RIGHT,
    ACT_PLACE,
    ACT_UNDO,
    ACT_PAUSE,
    ACT_SAVE,
    ACT_COUNT
} KeyAction;

typedef struct {
    int keys[ACT_COUNT];
    int musicOn;
    int musicVolume;
    int soundOn;
    int soundVolume;
    int turnTimeLimit;
    int gameTimeLimit;
} Settings;

typedef enum {
    SETTINGS_OK = 0,
    SETTINGS_ERR_OPEN,
    SETTINGS_ERR_FORMAT,
    SETTINGS_ERR_VERSION,
    SETTINGS_ERR_DATA,
    SETTINGS_ERR_NOT_FOUND,
    SETTINGS_ERR_KEY_CONFLICT
} SettingsResult;

void initSettings(Settings *settings);
SettingsResult setKeybind(Settings *settings, KeyAction action, int key);
SettingsResult setMusic(Settings *settings, int on, int volume);
SettingsResult setSound(Settings *settings, int on, int volume);
SettingsResult setTimeLimits(Settings *settings, int turnSeconds, int gameSeconds);
SettingsResult saveSettings(const Settings *settings);
SettingsResult loadSettings(Settings *settings);

const char *keyActionName(KeyAction action);
const char *settingsResultMessage(SettingsResult result);

#endif