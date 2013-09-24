//   pbuf.c 0.1  <Sep 2012>
//
//   Packet buffer (Linear List).
//

#include "veip.h"
#include <time.h>

/*
struct pbuf { // packet buffer
  struct pbuf * p_next; // next buf
  char * vp_pkt; // veip packet including inner ethernet frame
  time_t tm; // access time. shoud be usec

  struct vphdr vphdr; // index
};
*/

/* init_pbuf: Initialize of pbuf cell */
struct pbuf * init_pbuf(){
  struct pbuf * pbuf;
  pbuf = malloc(sizeof(struct pbuf));
  if(pbuf == NULL) return NULL;
  pbuf -> p_next = (struct pbuf *)NULL;
  pbuf -> vp_pkt = (char *)NULL;
  pbuf -> tm = (time_t)NULL;
  return pbuf;
}

/* set_pbuf: Set a pbuf cell in pbuf list*/
int set_pbuf(struct pbuf * pbuf, struct vphdr * vp, char * pkt, int len){
  struct pbuf * new_pbuf;
  if(pbuf == NULL) return(-1);

  if((new_pbuf = init_pbuf()) == NULL) return(-1);

  new_pbuf -> vphdr = *vp;

  new_pbuf -> vp_pkt = malloc(sizeof(u_char)*len);
  if(new_pbuf -> vp_pkt == NULL) return(-1);
  memcpy(new_pbuf -> vp_pkt, pkt, len);

  while(pbuf -> p_next != NULL){
    pbuf = pbuf -> p_next;
  }
  pbuf -> p_next = new_pbuf;

  return 0;
}

/* free_pbuf: Free a pbuf cell */
int free_pbuf(struct pbuf * pbuf){
  free(pbuf -> vp_pkt);
  free(pbuf);
  return 0;
}

/* freem_pbuf: Free pbuf list */
int freem_pbuf(struct pbuf * pbuf){
  struct pbuf * next_pbuf;
  if(pbuf == NULL) return(-1);

  pbuf = pbuf -> p_next;
  while(pbuf != NULL){
    next_pbuf = pbuf -> p_next;
    if(free_pbuf(pbuf) == -1) return(-1); //ToDo
    pbuf = next_pbuf;
  }
  return 0;
}

/* pbuf_cellcnt: Count pbuf cells in pbuf list */
int pbuf_cellcnt(struct pbuf * pbuf){
  int n = 0;
  if(pbuf == NULL) return(-1);

  pbuf = pbuf -> p_next;
  while(pbuf != NULL){
    pbuf = pbuf -> p_next;
    n++;
  }
  return n;
}

/* lookup_pbuf: Lookup a pbuf in pbuf list */
struct pbuf * lookup_pbuf(struct pbuf * pbuf, u_short vp_id){
  if(pbuf == NULL) return NULL;

  pbuf = pbuf -> p_next;
  while(pbuf != NULL){
    if(pbuf -> vphdr.vp_id == vp_id){
      return pbuf;
    }
    pbuf = pbuf -> p_next;
  }
  return NULL;
}

/* takeout_pbuf: Lookup and pullout a pbuf cell in pbuf list */
struct pbuf * takeout_pbuf(struct pbuf * pbuf, u_short vp_id){
  struct pbuf * pre_pbuf;
  if(pbuf == NULL) return NULL;
  pre_pbuf = pbuf;
  while(pbuf != NULL){
    if(pbuf -> vphdr.vp_id == vp_id){
      pre_pbuf -> p_next = pbuf -> p_next;
      return pbuf; //Must be free_pbuf(struct pbuf *) later.
    }
    pre_pbuf = pbuf;
    pbuf = pbuf -> p_next;
  }
  return NULL;
}

/* auto_pbuf: Takeout/set pbuf */
struct pbuf * auto_pbuf(struct pbuf * pbuf, struct vphdr * vp, char * pkt, int len){
  struct pbuf * pre_pbuf, * new_pbuf;
  if(pbuf == NULL) return NULL;
  pre_pbuf = pbuf;
  while(pbuf != NULL){
    if(pbuf -> vphdr.vp_id == vp -> vp_id){
      pre_pbuf -> p_next = pbuf -> p_next;
      return pbuf; //Must be free_pbuf(struct pbuf *) later.
    }
    pre_pbuf = pbuf;
    pbuf = pbuf -> p_next;
  }
  
  if((new_pbuf = init_pbuf()) == NULL) return NULL;

  new_pbuf -> vphdr = *vp;

  new_pbuf -> vp_pkt = malloc(sizeof(u_char)*len);
  if(new_pbuf -> vp_pkt == NULL) return NULL;
  memcpy(new_pbuf -> vp_pkt, pkt, len);

  pre_pbuf -> p_next = new_pbuf;
  return NULL;
}

