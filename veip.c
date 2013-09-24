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
//                  veip.c  0.2 - VEIP main program
//
// SYNOPSIS : x86 Centos6 
// DESC     : VEIP - Virtual Ethernet over IP protocol prototype.
//            Forward local fabric ethernet frame to remote fabric over 
//            IP network, such as the global Internet.
//
// LIBS     :
// CFLAGS   : -DLINUX
// AUTHER   : K.ISE <@ibucho>, T.ITO, 
// 
// USAGE    : veip -i dev -L log -f frag_size peer1 peer2 peer3 .....
//            'D' - debug mode enable.
//            'a' - append logging enable. default is "w" mode.
//            'i' - local network interface name.
//            'L' - Log file path name. default is ./log
//            'f' - fragment size. default is 1024 bye.
//
// HISTORY  : release 0.1 <Aug 2012>
//            二拠点間限定版。フラッグメントしないパケットの交換だけ実装
//            include: veip.c subr.c abuf.c
//
//            release 0.2 <Dec 2012>
//            マルチサイトをサポート。フラッグメント、デフラグを実装
//            include: veip.c subr.c abuf.c pbuf.c debug.c
//
#include "veip.h"

void usage(){
  fprintf(stderr,"usage:veip  -a -D -i dev -L log -f frag_size peer1 peer2 peer3 .....\n");
  return;
}

int frag_size = 1024;
char * logfn = "./log";

//===========================================================
// veip main 
//===========================================================

int main(int argc, char *argv[], char *envp){

  int ch;
  char * dev = "eth1"; // OCDETのGWはeth1がグローバル
  int debug = 0;
  int appnd = 0;

  while((ch = getopt(argc, argv, "Daf:i:L:")) != EOF){
    switch(ch){
    case 'D': // debug mode set
      debug = 1;
      break;
    case 'a':
      appnd =1; // enable append mode to logfile
      break;
    case 'f':
      frag_size = atoi(optarg);
      break;
    case 'i':
      dev = optarg;
      break;
    case 'L':
      logfn = optarg;
      break;
    case '?':
      fprintf(stderr,"unknown option\n");
      usage();
      return(-1);
    }
  }
  if(optind == argc){
    fprintf(stderr,"require remote veip-gw peer.\n");
    usage();
    return(-1);
  }

  // initializing log file
  // アペンドモードでなければログファイルをゼロクリアしとく
  FILE * lfp; 
  if(!appnd){
    if((lfp = fopen(logfn, "w")) < 0){
      perror("fopen");
      return;
    }
    fclose(lfp);
  }
  logging(logfn, "starting veip program .... ");

  // set peer name to vpeer struct.
  // pnumとpeer[]はグローバルexternにしたほうがいいかもしんない
  char str[LINLEN]; // ロギング用バッファ
  int pnum = 0;  // pnum + 1 がピアーの数
  struct vpeer peer[MAX_PEER_NUM];
  while(optind < argc) peer[pnum++].name = argv[optind++];
  in_addr_t vp_gw; // 32bit形式のIPアドレスだけど後方互換としてこんな名称
  char      ipaddr[INET_ADDRSTRLEN]; // inet_ntopに使うだけの文字列
  int i; // forループ用の共通カウンタ
  for(i = 0; i < pnum ; i++){
    if(set_peer(&peer[i]) < 0){
      logging(logfn, "set_peer error\n");
      return(-1);
    }
    vp_gw = peer[i].vp_gw;
    inet_ntop(PF_INET, &vp_gw, ipaddr, sizeof(ipaddr));
    memset(str, 0, sizeof(str));
    sprintf(str, "registered veip peer : %s [%s]", peer[i].name,ipaddr);
    logging(logfn, str);
  }

  // create socket with port VEIP, to recv packets.
  // リモートVEIPピアーからの受信用
  int recv;
  if((recv= crecvsock(VEIP)) < 0){
    memset(str, 0, sizeof(str));
    sprintf(str, "can not get sockdet port %d to recv.", VEIP);
    logging(logfn, str);
    return(-1);
  }
  // create network interface discriptor
  // マルチdevインターフェースにするにはここも関数化必要
  int nif; 
  if((nif = get_nif(dev)) < 0){ // init local device
    memset(str, 0, sizeof(str));
    sprintf(str,"can not open local device %s.", dev);
    logging(logfn, str);
    return(-1);
  }
  int gfd = recv;  // select用最大記述子番号
  if(nif > recv) gfd = nif;

  // create socket with port CNTL, to communicate manager
  // マネージャとの通信用ポート(0.2では未実装)
  //
  int ctlp;
  if((ctlp= crecvsock(CNTL)) < 0){
    memset(str, 0, sizeof(str));
    sprintf(str, "can not get sockdet port %d to recv.", CNTL);
    logging(logfn, str);
    return(-1);
  }
  if(ctlp > gfd) gfd = ctlp;

  // selectに nif, recv, ctlpをセット
  fd_set fd, rfd;
  FD_ZERO(&rfd);
  FD_SET(nif, &rfd);
  FD_SET(recv, &rfd);
  FD_SET(ctlp, &rfd);

  // ヘッダ解析用バッファを準備しとく
  struct ether_header * ehdr;
  ehdr = malloc(sizeof(struct ether_header));
  if(ehdr == NULL) return(-1);

  struct ether_arp    * earp;
  earp = malloc(sizeof(struct ether_arp));
  if(earp == NULL) return(-1);

  // パケットバッファとパケット長、abufポインタ作っとく
  u_char pbuf[ETHER_MAX_LEN + 64];
  u_int len;
  struct abuf * ac = (struct abuf *)a_get();
  if(ac == NULL) return(-1);

  // recvfromで受信するためのパラメタ
  struct sockaddr_in s_addr;
  int s_size = sizeof(struct sockaddr_in);

  init_pbuf_list(); // test added by ito
  struct abuf * ap; // 転送先vp_gwを探すためのabufポインタ
  int cnt = 0;      // 受信パケットカウンタ

  // ここから無限ループ。forkして親は戻る。
  // いずれsignalで終了処理を実装せねば
  int pid;
  if((pid = fork()) < 0){
    memset(str, 0, sizeof(str));
    sprintf(str,"can not fork the process.");
    logging(logfn, str);
    return(-1);
  }
  if(pid){
    return(0);
  }
    
  while(TRUE){
    memcpy(&fd, &rfd, sizeof(fd_set));
    if(select(gfd+1, &fd, NULL, NULL, NULL) < 0) continue;
    cnt++;
    memset(pbuf, 0, sizeof(pbuf));
    memset(ehdr, 0, sizeof(struct ether_header));
    //*******************************************************
    // VEIPピアーからパケットを受け取った時の処理
    // a_lookup_ethや、いずれVLANマッピングする時に送信元が
    // 必要なのでrecvfromで受け取る
    //*******************************************************
    if(FD_ISSET(recv, &fd)){
      memset(&s_addr, 0, sizeof(struct sockaddr_in));
      len = recvfrom(recv, pbuf, sizeof(pbuf), 0, (struct sockaddr *)&s_addr, &s_size);
      if(len < 0) continue;
      vp_gw = s_addr.sin_addr.s_addr;
      int f = recveip(ac, pbuf, len, vp_gw, nif);
      if(f < 0) continue;
      if(debug){
        memset(str, 0, sizeof(str));
        inet_ntop(PF_INET, &vp_gw, ipaddr, sizeof(ipaddr));
        sprintf(str,"%04d received packet from [%s] %d byte.", cnt, ipaddr, len);
        logging(logfn, str);
      }
      continue;
    }

    //************************************************************
    // devインターフェースからパケットを受け取った時の処理
    //************************************************************
    if(FD_ISSET(nif, &fd)){
      len = read(nif, pbuf, sizeof(pbuf));
      if(len < 0) continue;
      memcpy(ehdr, pbuf, sizeof(struct ether_header));

      if(ntohs(ehdr->ether_type) == ETHERTYPE_VLAN){
        // タグVLANだったら
        // いまんとこ何もしない
        continue;
      }

      // IPパケットだったら
      if(ntohs(ehdr->ether_type) == ETHERTYPE_IP){
        // src dstの mac が0x00や0xffだったら無視する
        if(ehackzero(ehdr->ether_dhost) || ehackzero(ehdr->ether_shost)) continue;
        if(ehackbcas(ehdr->ether_dhost) || ehackbcas(ehdr->ether_shost)) continue;

        ap = (struct abuf *)a_lookup_eth(ac, ehdr->ether_dhost);
        if (ap == NULL) continue;

        for(i = 0; i < pnum ; i++){
          if(ap->target.vp_gw == peer[i].vp_gw){
            if(sendveip(peer[i].sock, peer[i].name, pbuf, len) <0) continue;
            if(debug){
              memset(str, 0, sizeof(str));
              vp_gw = peer[i].vp_gw;
              inet_ntop(PF_INET, &vp_gw, ipaddr, sizeof(ipaddr));
              sprintf(str,"%04d send packet to %s [%s]", cnt, peer[i].name, ipaddr);
              logging(logfn, str);
            } // if(debug)
            break;
          }
        } // for(i = 0; i < pnum ; i++){
        continue;
      }  // if(ntohs(ehdr->ether_type) == ETHERTYPE_IP){

      // ARPパケットだったら
      if(ntohs(ehdr->ether_type) == ETHERTYPE_ARP){
        memset(earp, 0, sizeof(struct ether_arp));
        memcpy(earp, pbuf + sizeof(struct ether_header), sizeof(struct ether_arp));

        switch(ntohs(earp->arp_op)){
        case ARPOP_REQUEST: // これARPリクエストやからブロードキャストせな
          for(i = 0; i < pnum ; i++){
            (void)sendveip(peer[i].sock, peer[i].name, pbuf, len);
            if(debug){
              memset(str, 0, sizeof(str));
              vp_gw = peer[i].vp_gw;
              inet_ntop(PF_INET, &vp_gw, ipaddr, sizeof(ipaddr));
              sprintf(str,"%04d arp request sent to %s [%s]", cnt, peer[i].name, ipaddr);
              logging(logfn, str);
            } // if(debug)
          }
          break;
        case ARPOP_REPLY: // これARPの返事やからvp_gw宛てに返さな
          ap = (struct abuf *)a_lookup_eth(ac, earp->arp_tha);
          if (ap <= 0) {
            if(debug){
              memset(str, 0, sizeof(str));
              sprintf(str,"%04d no such mac addr for ", cnt);
              logging(logfn, str);
              if((lfp = fopen(logfn, "a")) < 0){
                break;
              }
              fprintf(lfp, " -> ");
              pri_eha(lfp, earp->arp_tha);
              fprintf(lfp, "\n");
              fclose(lfp);
            } // if(debug)
            break;
          }
          for(i = 0; i < pnum ; i++){ 
            if(ap->target.vp_gw == peer[i].vp_gw){ // vp_gwどれや？
              if(sendveip(peer[i].sock, peer[i].name, pbuf, len) < 0) break;
              else{
                if(debug){
                  memset(str, 0, sizeof(str));
                  vp_gw = peer[i].vp_gw;
                  inet_ntop(PF_INET, &vp_gw, ipaddr, sizeof(ipaddr));
                  sprintf(str," arp reply back to %s [%s]", peer[i].name, ipaddr);
                  logging(logfn, str);
                  if((lfp = fopen(logfn, "a")) < 0){
                    break;
                  }
                  fprintf(lfp, " -> ");
                  pri_eha(lfp, earp->arp_sha);
                  fprintf(lfp, "\n");
                  fclose(lfp);
                } // if(debug)
                break; // 見つかったのでループ抜ける
              }
            }
          }
          break; 
        default:
          break;
        } // switch
        continue;
      } // if(ntohs(ehdr->ether_type) == ETHERTYPE_ARP)
    } // if(FD_ISSET(nif, &fd))
  } // while(TRUE)

  // ここまで来ないけど念のため
  return(0);
}

// -- end of code
