#include "Player.h"
#include <string.h>
void Player_init(Player* p, const char* name, Cell symbol) {
    if (!p) return;
    p->symbol=symbol;
    Player_setName(p,name);
}
int Player_setName(Player* p, const char* name) {
    size_t n; if(!p||!name||!name[0]) return 0;
    n=strlen(name); if(n>=PLAYER_NAME_MAX)n=PLAYER_NAME_MAX-1;
    memcpy(p->name,name,n); p->name[n]='\0'; return 1;
}
