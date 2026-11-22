#include "Stats.h"
#include <stdio.h>
void Stats_reset(PlayerStats* s) { if (!s) return; s->moves=s->wins=s->losses=s->draws=0; }
void Stats_recordMove(PlayerStats* s) { if (s) ++s->moves; }
void Stats_recordWin(PlayerStats* w, PlayerStats* l) { if(w)++w->wins; if(l)++l->losses; }
void Stats_recordDraw(PlayerStats* x, PlayerStats* o) { if(x)++x->draws; if(o)++o->draws; }
int Stats_save(const char* path, const PlayerStats* x, const PlayerStats* o) {
    FILE* f; if(!path||!x||!o) return 0; f=fopen(path,"w"); if(!f)return 0;
    fprintf(f,"%d %d %d %d\n%d %d %d %d\n",x->moves,x->wins,x->losses,x->draws,o->moves,o->wins,o->losses,o->draws);
    return fclose(f)==0;
}
int Stats_load(const char* path, PlayerStats* x, PlayerStats* o) {
    FILE* f; PlayerStats a,b; if(!path||!x||!o)return 0; f=fopen(path,"r"); if(!f)return 0;
    if(fscanf(f,"%d%d%d%d%d%d%d%d",&a.moves,&a.wins,&a.losses,&a.draws,&b.moves,&b.wins,&b.losses,&b.draws)!=8){fclose(f);return 0;}
    fclose(f); *x=a; *o=b; return 1;
}
