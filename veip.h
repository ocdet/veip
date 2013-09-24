// Copyright(C) 2012, Open Cloud Demonstration Experiments Taskforce.
//
// veip.h 0.1
//
// VEIP header files.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netdb.h>
#include <time.h>

// Linux dependent
#ifdef LINUX
#include <linux/if.h>
#include <netpacket/packet.h>
#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6 ETH_P_IPV6
#endif
#ifndef ETHERTYPE_VLAN
#define ETHERTYPE_VLAN ETH_P_8021Q
#endif
#endif

// BSD dependent
#ifdef BSD

#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define VEIP 59999 // VEIP port number, unofficial
#define CNTL 60001 // VEIP-GW control port, unofficial
#define MAX_PEER_NUM 32 // max veip peer number
#define LINLEN 128  // max line length of CLI or LOG
#define BUFS 2048 // buf size for test
#define PORT 6111 // UDP port for test

// ARP Table buffer (like mbuf)
struct abuf {   // address buffer
  struct abuf * a_next;
  struct abuf * nxtbuf;
  time_t tm;    // access time.
  //in_addr_t vp_gw; // remote gateway ip addr]
  // shoud be union, remote gw and local dev like below
  
  union {
   in_addr_t vp_gw; // remote gateway in ip addr
   char * device;   // local device name like 'eth0'
  } target;
  
  in_addr_t ip;  // dst ip address
  u_char eha[ETHER_ADDR_LEN]; // dst ethernet address
  u_char pad[2];  // 32 bit boundary.
};

// VEIP header
struct vphdr {
  u_short  vp_frg;             // fragment field
#define NO_FRGMNT    0x1000   // no fragment packet
#define FRAG_STRT    0x2000   // fragmented start packet
#define FRAG_STOP    0x4000   // fragmented stop packet
#define ADDR_RSOL    0x8000   // address resolution packet
#define OFST_MASK    0x0FFF   // offset mask
  u_short  vp_len;             // payload length excluding hdr
  u_short  vp_id;              // veip ID
  u_short  vp_pad;             // reserved, 32 bit boundary
};

// Packet buffer
struct pbuf { // packet buffer
  struct pbuf * p_next; // next buf
  char * vp_pkt; // veip packet including inner ethernet frame
  time_t tm; // access time. shoud be usec

  struct vphdr vphdr; // index
};

// VEIP-GW remote Peer with sending packet 
struct vpeer {
  char * name; // peer name;
  in_addr_t vp_gw; // IP address in 32bit u_int.
  int sock; // sockdet descriptor to send packet.
};
