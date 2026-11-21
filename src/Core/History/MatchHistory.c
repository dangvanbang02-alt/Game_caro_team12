#include "MatchHistory.h"
#include <stdio.h>
#include <string.h>
void MatchHistory_init(MatchHistory* h){if(h)h->count=0;}
int MatchHistory_add(MatchHistory* h,const MatchRecord* r){if(!h||!r||h->count>=MAX_MATCH_HISTORY)return 0;h->records[h->count++]=*r;return 1;}
int MatchHistory_save(const char* path,const MatchHistory* h){FILE*f;int i;if(!path||!h)return 0;f=fopen(path,"w");if(!f)return 0;for(i=0;i<h->count;++i){const MatchRecord*r=&h->records[i];fprintf(f,"%s|%s|%d|%d|%lu\n",r->playerX,r->playerO,(int)r->result,r->moveCount,r->durationMs);}return fclose(f)==0;}
int MatchHistory_load(const char* path,MatchHistory* h){FILE*f;char line[256];MatchHistory tmp;if(!path||!h)return 0;f=fopen(path,"r");if(!f)return 0;MatchHistory_init(&tmp);while(fgets(line,sizeof(line),f)&&tmp.count<MAX_MATCH_HISTORY){MatchRecord r;int result;unsigned long duration;char *a=strchr(line,'|'),*b,*c,*d;if(!a)continue;*a++='\0';b=strchr(a,'|');if(!b)continue;*b++='\0';c=strchr(b,'|');if(!c)continue;*c++='\0';d=strchr(c,'|');if(!d)continue;*d++='\0';memset(&r,0,sizeof(r));strncpy(r.playerX,line,sizeof(r.playerX)-1);strncpy(r.playerO,a,sizeof(r.playerO)-1);if(sscanf(b,"%d",&result)!=1||sscanf(c,"%d",&r.moveCount)!=1||sscanf(d,"%lu",&duration)!=1)continue;r.result=(GameResult)result;r.durationMs=duration;tmp.records[tmp.count++]=r;}fclose(f);*h=tmp;return 1;}
