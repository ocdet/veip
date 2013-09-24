#include "veip.h"
int frag_size = 1024;
char * logfn = "./log";

//===========================================================
// recv.c recieving 2048 packet from remote program
//===========================================================
int main(int argc, char *argv[], char *envp){
  int sd; // listen discrptor to recv from rmt
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sd < 0){
    perror("socket");
    return(-1);
  }
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
    perror("bind");
    return(-1);
  }

  FILE * lfp;
  if((lfp = fopen(logfn, "w")) < 0){
    perror("fopen");
    return;
  }
  fclose(lfp);
  u_char pbuf[BUFS];
  int m;
  char str[LINLEN];

  while(1){
    memset(pbuf, 0, sizeof(pbuf));
    printf("listening.\n");    
    if((m = recv(sd, pbuf, sizeof(pbuf),0)) < 0){
      continue;
    }
    memset(str, 0, sizeof(str));
    sprintf(str, "%d byte recv, port %d", m, PORT);
    logging(logfn, str);
  }
}
