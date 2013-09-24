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
//
// subr.c 0.2  <Dec 2012>
//
// subrotines for VEIP.
//
//   
#include "veip.h"


struct pbuf * pbuf_start_list;
struct pbuf * pbuf_stop_list;
extern int frag_size;
extern char * logfn;

int init_pbuf_list(){
  pbuf_start_list = init_pbuf();
  pbuf_stop_list = init_pbuf();
  return 0;
}

//===========================================================
// return 16 bit internet checksum
//===========================================================
#define CMPLS(x) (x = (x & 0xffff) + (x >> 16)) // 1's complement sum
u_int16_t cksum(void * data, u_int len){
  u_int32_t sum = 0;
  union {
    u_int8_t s_char[2];
    u_int16_t d_char;
  } * p_data = data, odd = { .s_char[1] = 0 /* padding for odd data */};

  for( ; len > 1; p_data++, len -= 2, CMPLS(sum))
    sum += p_data->d_char;

  if(len > 0){ // if last u_short in odd
    odd.s_char[0] = p_data->s_char[0];
    sum += odd.d_char;
    CMPLS(sum);
  }
  return ~(u_int16_t)sum; // return 1's complement "of" sum.
}

//===========================================================
// split token by delimiter
//===========================================================
void split(char str[], char * delim){
  char * tok[LINLEN];
  char * ptr = str;
  int i, len;
  for (len = 0; len < LINLEN; len++){
    if((tok[len] = strtok(ptr, delim)) == NULL)
      break;
    ptr = NULL;
  }
  memcpy(str, tok, LINLEN);
  return;
}

//===========================================================
// get in_addr_t from char * peer
//===========================================================
in_addr_t get_inaddr(char * peer){
  in_addr_t addr;
  struct hostent * hp;

  addr = inet_addr(peer);
  if( addr == 0xffffffff){ // inet_addr return -1 if error.
    hp = (struct hostent *)gethostbyname(peer);
    if(!hp) return(-1);
    addr = *(unsigned int *)hp->h_addr;
  }
  return(addr);
}

//===========================================================
// etype : return ether_type in network byte order.
//===========================================================
u_short etype(char * pkt){
  struct ether_header * ehdr;
  u_short type;
  ehdr = (struct ether_header *)malloc(sizeof(struct ether_header));
  if(ehdr == NULL) return(0);
  memset(ehdr, 0, sizeof(struct ether_header));
  memcpy(ehdr, pkt, sizeof(struct ether_header));
  type = ehdr->ether_type;
  free(ehdr);
  return(ntohs(type));
}


//===========================================================
// get network interface discriptor by device name,
//   : Linux version
//===========================================================
int get_nif(char * device){
  struct ifreq ifreq;
  struct sockaddr_ll sa;
  int sock;
  // generate socket to capture frames.
  if((sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))<0){
    perror("socket");
    return(-1);
  }
  // get interface index
  memset(&ifreq, 0, sizeof(struct ifreq));
  strncpy(ifreq.ifr_name, device, sizeof(ifreq.ifr_name)-1);
  if(ioctl(sock, SIOCGIFINDEX, &ifreq)<0){
    perror("ioctl");
    close(sock);
    return(-1);
  }
  // set sockaddr parameters
  sa.sll_family = PF_PACKET;
  sa.sll_protocol = htons(ETH_P_ALL);
  sa.sll_ifindex = ifreq.ifr_ifindex;
  // bind generated socket and sockaddr
  if(bind(sock, (struct sockaddr *)&sa, sizeof(sa))<0){
    perror("bind");
    close(sock);
    return(-1);
  }
  // get interface flags
  if(ioctl(sock, SIOCGIFFLAGS, &ifreq)<0){
    perror("ioctl");
    close(sock);
    return(-1);
  }
  // set interface as promiscuous mode
  ifreq.ifr_flags=ifreq.ifr_flags|IFF_PROMISC;
  if(ioctl(sock, SIOCSIFFLAGS, &ifreq)<0){
    perror("ioctl");
    close(sock);
    return(-1);
  }
  return(sock);
}

//===========================================================
// senddgm : send n byte *pkt to *peer with UDP
//===========================================================
int senddgm(int sock, char * peer, int port, char * pkt, int n){
  struct sockaddr_in addr;
  struct hostent * hp;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = get_inaddr(peer);
  if(addr.sin_addr.s_addr < 0){
    perror("get_inaddr");
    return(-1);
  }
  int m = 0; // sended byte
  m = sendto(sock, pkt, n, 0, (struct sockaddr *)&addr, sizeof(addr));
  if(m != n){
    perror("sendto");
    return(-1);
  }
  return(m);
}

//===========================================================
// sendveip : send VEIP frame to VEIP-GW
//===========================================================
int sendveip(int sock,char * peer, char * pkt, int n){
  struct vphdr * vp;
  struct ether_header * ehdr;
  char * p;
  u_short offs = 0; // fragment offset
  int ret;
  
  ehdr = malloc(sizeof(struct ether_header));
  if(ehdr == NULL) return(-1);
  memcpy(ehdr, (struct ether_header *)pkt, sizeof(struct ether_header));

  vp = malloc(sizeof(struct vphdr));
  if(vp == NULL){
    free(ehdr);
    return(-1);
  }

  if(ntohs(ehdr->ether_type) == ETHERTYPE_ARP){
    vp->vp_frg = htons(ADDR_RSOL);
  }else if(ntohs(ehdr->ether_type) == ETHERTYPE_REVARP){
    vp->vp_frg = htons(ADDR_RSOL); // just in case lol
  }else{
    // this prototype doesn't support fragmentations.
    vp->vp_frg = htons(NO_FRGMNT);
  }

  vp->vp_len = htons(n);
  vp->vp_id = cksum(pkt, n); // temporary, use 16bit checksum as ID
  vp->vp_pad = 0x0000;
  p = malloc(sizeof(struct vphdr) + n);
  if(p == NULL){
    free(vp); free(ehdr);
    return(-1);
  }
  memset(p, 0, sizeof(struct vphdr) + n);
  memcpy(p, vp, sizeof(struct vphdr));

  int m = 0;
  if(n > frag_size){
    m = frag_size;
    vp->vp_frg = htons(FRAG_STRT);
    vp->vp_len = htons(m);
    memcpy(p, vp, sizeof(struct vphdr));

    memcpy(p + sizeof(struct vphdr), pkt, m);
    ret = senddgm(sock, peer, VEIP, p, sizeof(struct vphdr) + m);
    if(ret < 0){
      free(vp); free(ehdr); free(p);
      return(-1);
    }
    vp->vp_frg = htons(FRAG_STOP);
    vp->vp_len = htons(n - m);
    memcpy(p, vp, sizeof(struct vphdr));
  }
  free(vp); free(ehdr);

  memcpy(p + sizeof(struct vphdr), pkt + m, n - m);
  ret = senddgm(sock, peer, VEIP, p, sizeof(struct vphdr) + n - m);
  free(p);
  if(ret < 0) return(-1);
  return(ret);
}

//===========================================================
// receiving packet process from VEIP-GW 
//===========================================================
int recveip(struct abuf * c, char * pkt, int len, in_addr_t vp_gw, int nif){
  FILE * lfp;
  int ret;
  struct vphdr * vp;
  struct pbuf * take_pbuf; //test
  char * p; //test
  vp = malloc(sizeof(struct vphdr));
  if(vp == NULL) return(-1);
  memset(vp, 0, sizeof(struct vphdr));
  memcpy(vp, pkt, sizeof(struct vphdr));
  // if packet was fragmented.
  if(ntohs(vp->vp_frg) & FRAG_STRT){
    take_pbuf = takeout_pbuf(pbuf_stop_list, vp -> vp_id);
    if(take_pbuf != NULL){
      // ToDo: pkt(start) + take_pbuf(stop)
      p = malloc(sizeof(char)*(len - sizeof(struct vphdr) + ntohs(take_pbuf -> vphdr.vp_len)));
      memcpy(p, pkt + sizeof(struct vphdr), len - sizeof(struct vphdr));
      memcpy(p + len - sizeof(struct vphdr), take_pbuf -> vp_pkt + sizeof(struct vphdr), ntohs(take_pbuf -> vphdr.vp_len));
      ret = write(nif, p, len - sizeof(struct vphdr) + ntohs(take_pbuf -> vphdr.vp_len));
      free(p);
      free(take_pbuf);
    }
    else{
      if(set_pbuf(pbuf_start_list, vp, pkt, len) == -1) return(-1);
    }
  }
  if(ntohs(vp->vp_frg) & FRAG_STOP){
    take_pbuf = takeout_pbuf(pbuf_start_list, vp -> vp_id);
    if(take_pbuf != NULL){
      p = malloc(sizeof(char)*(ntohs(take_pbuf -> vphdr.vp_len) + len - sizeof(struct vphdr)));
      memcpy(p, take_pbuf -> vp_pkt + sizeof(struct vphdr), ntohs(take_pbuf -> vphdr.vp_len));
      memcpy(p + ntohs(take_pbuf -> vphdr.vp_len), pkt + sizeof(struct vphdr), len - sizeof(struct vphdr));
      ret = write(nif, p, ntohs(take_pbuf -> vphdr.vp_len) + len - sizeof(struct vphdr));
      free(p);
      free(take_pbuf);
    }
    else{
      if(set_pbuf(pbuf_stop_list, vp, pkt, len) == -1) return(-1);
    }
  }

  // if no fragment packet, pass thought it to device.
  if(ntohs(vp->vp_frg) & NO_FRGMNT){
    ret = write(nif, pkt + sizeof(struct vphdr),len - sizeof(struct vphdr));
  }
  if(ret < 0){
    perror("write");
    free(vp);
    return(-1);
  }

  // If packet from GW is ARP(ADDR_RSOL),register new remote MAC address
  // into abuf chain.
  //
  if(ntohs(vp->vp_frg) & ADDR_RSOL){
    struct ether_arp  * earp;
    earp = malloc(sizeof(struct ether_arp));
    if(earp == NULL){
      free(vp);
      return(-1);
    }
    memset(earp, 0, sizeof(struct ether_arp));
    memcpy(earp, pkt + sizeof(struct vphdr) + sizeof(struct ether_header),
           sizeof(struct ether_arp));
    in_addr_t ip;
    memcpy(&ip, &earp->arp_spa, 4);

    // check src ether is registered. if not, registger it.
    struct abuf * a = (struct abuf *)a_lookup_eth(c, earp->arp_sha);
    if(a == NULL){
      if(c->target.vp_gw == 0){ // no arp cache
        memcpy(c->eha, earp->arp_sha, ETHER_ADDR_LEN);
        c->target.vp_gw = vp_gw;
        c->ip = ip;
        c->tm = time(NULL);
      }else{
        ret = reg_eha(c, earp->arp_sha, vp_gw, ip);
        if(ret < 0){
          free(earp);
          free(vp);
          return(-1);
        }
        // if(debug){
        if((lfp = fopen(logfn, "a")) < 0);
        a_list(lfp, c);
        fclose(lfp);
        // }

      }
    }
    ret = write(nif, pkt + sizeof(struct vphdr),len - sizeof(struct vphdr));
    if(ret < 0){
      perror("write");
      free(earp);
      free(vp);
      return(-1);
    }
    free(earp);
  } // ADDR_RSOL

  free(vp);
  return(0);
}

//===============================================================
// create udp socket to listen
//===============================================================
int crecvsock(int port){
  int rv;
  struct sockaddr_in saddr;
  rv = socket(AF_INET, SOCK_DGRAM, 0);
  if(rv < 0) return(-1);
  
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(rv, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) return(-1);
  return(rv);
}

//===============================================================
// set address and get socket for remote veip peer
//===============================================================
int set_peer(struct vpeer * peer){
  // in_addr_t of peer
  peer->vp_gw = get_inaddr(peer->name);
  if(peer->vp_gw < 0){
    return(-1);
  }
  // socket to send
  if((peer->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    return(-1);
  }
  return(1);
}

//===========================================================
// register new mac address into ARP table
//===========================================================

int reg_eha(struct abuf * c, u_char eha[], in_addr_t vp_gw, in_addr_t ip){
  struct abuf * a;
  a = a_get();
  if ( a == NULL) return(-1);
  memcpy(a->eha, eha, ETHER_ADDR_LEN);
  a->target.vp_gw = vp_gw;
  a->ip = ip;
  a_append(c, a);
  return(NULL);
}

//===========================================================
// check mac of 0x00 and 0xff. if so, return TRUE else FALSE
//===========================================================

int ehackzero(u_char eha[]){
  u_char zero[6] = {};
  if(memcmp(eha, zero, ETHER_ADDR_LEN) == 0)
    return(TRUE);
  return(FALSE);
}


int ehackbcas(u_char eha[]){
  u_char bcas[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
  if(memcmp(eha, bcas, ETHER_ADDR_LEN) == 0)
    return(TRUE);
  return(FALSE);
}
