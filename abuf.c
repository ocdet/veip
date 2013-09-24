/*
 * Copyright (c) 2012,2013 Open Cloud Demonstration Experiments Taskforce.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
// Copyright(C) 2012, Open Cloud Demonstration Experiments Taskforce.
// abuf.c 0.1 <Aug 2012>
//
// ARP address table buffer which is based on mbuf of BSD 
// architecture.
//   
#include "veip.h" 

//===========================================================
//
// abuf functions for ARP address table
//
//===========================================================
struct abuf * a_get(){
  struct abuf * a;
  a = malloc(sizeof(struct abuf));
  if(a == NULL) return(NULL);
  memset(a, 0, sizeof(struct abuf));
  a->a_next= (struct abuf *)NULL;
  a->nxtbuf = (struct abuf *)NULL;
  a->tm = time(NULL);
  return(a);
}

void a_free(struct abuf * a){
  free(a);
  return;
}

void a_freem(struct abuf * a){
  struct abuf * b;
  while(a->a_next){
    b = a;
    a = a->a_next;
    a_free(b);
  }
  return;
}

int a_cellcnt(struct abuf * c){
  int i = 0;
  while(c != NULL){
    c = c->a_next;
    i++;
  }
  return(i);
}
//===========================================================
// prepend, append new abuf 'a' to abuf chain 'c'.
//===========================================================
void a_append(struct abuf * c, struct abuf * a){
  while(c->a_next) c = c->a_next;
  c->a_next= a;
  a->tm = time(NULL);
  return;
}

struct abuf * a_prepend(struct abuf * c, struct abuf * a){
  if(a->a_next)return(NULL); // behind cell exist.
  a->a_next= c;
  a->tm = time(NULL);
  return(a);
}

//===========================================================
// pull out a abuf 'a' from chain 'c',
//===========================================================
int a_pullout(struct abuf * c, struct abuf * a){
  if(c == a){
    if(a->a_next){
      c->a_next = a->a_next;
      a->a_next= NULL;
    }
    return(1);
  }

  if(a_lookup_buf(c,a) < 0 ) return(-1);
  do{
    if(c->a_next == a){
      c->a_next= a->a_next;
      a->a_next= NULL;
      return(1);
    }
    c = c->a_next;
  }while(c != NULL);
}

//===========================================================
// lookup abuf 'a'  which has the eha[] in chain 'c'.
//===========================================================
struct abuf * a_lookup_eth(struct abuf * c, u_char eha[]){
  do{
    if(memcmp(eha, c->eha, ETHER_ADDR_LEN) == 0){
      if( c != 0x0 ) return(c);
    }
    c = c->a_next;
  }while(c);
  return(NULL);
}


//===========================================================
// lookup a  abuf cell'a' in chain 'c'
//===========================================================
int  a_lookup_buf(struct abuf * c, struct abuf * a){
  int i = 0; // index counter
  do{
    if(c == a) return(i);
    i++;
    c = c->a_next;
  }while(c != NULL);
  return(-1);
}
//-- end of code
