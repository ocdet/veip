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
// debug.c 0.1 <Dec 2012>
//
// print out functions for debug of VEIP.
//
//   
#include "veip.h"
//===========================================================
//
//
// Optional functions for debug
//
//
//===========================================================
// list abuf table
//===========================================================
void a_list(FILE * fd, struct abuf * c){

  int i = 0; // index
  char vp_gw[INET_ADDRSTRLEN],ip[INET_ADDRSTRLEN];
  do{
    fprintf(fd, "%04d ", i++);
    pri_eha(fd, c->eha);
    inet_ntop(PF_INET, &c->ip, ip, sizeof(ip));
    inet_ntop(PF_INET, &c->target.vp_gw, vp_gw, sizeof(vp_gw));
    fprintf(fd, "(%s) via %s \n", ip, vp_gw);
    c=c->a_next;
  }while(c != NULL);
  return;
}

//===========================================================
// Print ethernet hardware address in "ff:ff:ff:ff:ff:ff".
//===========================================================
void pri_eha(FILE * fd, u_char eha[]){
  int i;
  for(i = 0; i < ETHER_ADDR_LEN; i++){
    fprintf(fd,"%02x", eha[i]);
    if( i < (ETHER_ADDR_LEN -1)) fprintf(fd,":");
  }
  return;
}

//===========================================================
// Print ethernet header summary
//===========================================================
void pri_ehdr(FILE * fd, struct ether_header * hdr){
  pri_eha(fd, hdr->ether_shost);
  fprintf(fd, " > ");
  pri_eha(fd, hdr->ether_dhost);
  fprintf(fd, " ethertype ");
  switch(ntohs(hdr->ether_type)){
  case ETHERTYPE_IP:
    fprintf(fd,"IPv4(0x%04x)", ntohs(hdr->ether_type));
    break;
  case ETHERTYPE_IPV6:
    fprintf(fd,"IPv6(0x%04x)", ntohs(hdr->ether_type));
    break;
  case ETHERTYPE_ARP:
    fprintf(fd, "ARP(0x%04x)", ntohs(hdr->ether_type));
    break;
  case ETHERTYPE_VLAN:
    fprintf(fd, "VLAN(0x%04x)", ntohs(hdr->ether_type));
    break;
  default:
    fprintf(fd, "0x%04x", ntohs(hdr->ether_type));
    break;
  }
  return;
}

//===========================================================
// Print IP header summary.
//===========================================================
void pri_iphdr(FILE * fd, struct iphdr * hdr){
  char src[INET_ADDRSTRLEN],dst[INET_ADDRSTRLEN];
  inet_ntop(PF_INET, &hdr->saddr, src, sizeof(src));
  inet_ntop(PF_INET, &hdr->daddr, dst, sizeof(dst));
  fprintf(fd,
          "IP (tos 0x%02x, ttl %d, id %d, frag 0x%04x, proto %d, length: %d)\n%s > %s",
          hdr->tos,      // type of service
          hdr->ttl,      // time to live
          ntohs(hdr->id),// ID
          ntohs(hdr->frag_off), // fragment flag and offset
          hdr->protocol, // payload protocol
          ntohs(hdr->tot_len), // total length (including IP header)
          src, dst); 
  return;
}  

//===========================================================
// Print packet data in Hex
//===========================================================
void pri_x(FILE * fd, char * ptr, int len){
  int i;
  u_char pbuf[len];
  memcpy(pbuf, ptr, len);
  for( i = 0; i < len; i++){
    if(!(i % 16)){
      if( i > 0 )  fprintf(fd, "\n");
      fprintf(fd,"%04d ", i);
    }
    fprintf(fd,"%02x",pbuf[i]);
    if(i % 2) fprintf(fd," ");
  }
  fprintf(fd,"\n");
  return;
}

//===========================================================
// logging
//===========================================================
void logging(char f[], char m[]){
  FILE * fd;
  if((fd = fopen(f, "a")) < 0){
    perror("fopen");
    return;
  }
  time_t ctime;
  struct tm * tm;
  ctime = time(NULL);
  tm = (struct tm *)localtime(&ctime);
  fprintf(fd, "%d/%02d/%02d %02d:%02d:%02d ", 
          tm->tm_year + 1900,
          tm->tm_mon +1,
          tm->tm_mday,
          tm->tm_hour,
          tm->tm_min,
          tm->tm_sec);
  fprintf(fd, "%s\n", m);
  fclose(fd);
  return;
}



/////////////////////////////////////////////
// END of debug.c
