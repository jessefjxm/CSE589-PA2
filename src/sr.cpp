#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
struct msgcache {
   struct msg* m;
   int acked;
   int time;
};

#define T_INTERVAL 50       // how long can one packet wait
#define CHECK_INTERVAL 10   // how long a TICK is

int seqnum[2];
int acknum[2];
struct msgcache* bufmsg[1001];

int nextseqnum, base, N;

int baseB;
struct msg* bufdata[1001];

static int checkSum(struct pkt* packet){
  int s = packet->seqnum + packet->acknum;
  for(int i=0;i<20;i++){
    s += packet->payload[i];
  }
  return s;
}

static int checkLen(char* str){
  int len = 0;
  for(;len<20 && str[len]!='\0'; len++);
  return len;
}

static pkt* makePacket(int nextseqnum, int nextacknum, char* data){
  struct pkt* packet = new pkt();
  packet->seqnum = nextseqnum;
  packet->acknum = nextacknum;
  strncpy(packet->payload, data, 20);
  packet->checksum = checkSum(packet);
  return packet;
}

static void sendPacket(int type, struct pkt *packet){
  if(type == 0){
    printf("[A - send] %d,%d,%.20s\n", packet->seqnum, packet->acknum, packet->payload);
  }else{
    printf("[B - ACK] %d|%d,%d,%.20s\n", baseB, packet->seqnum, packet->acknum, packet->payload);
  }
  tolayer3(type, *packet);
}

static int min(int a, int b){
  return a<b?a:b;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
  struct msg* m = new msg();
  strncpy(m->data, message.data, 20);
  struct msgcache* mc = new msgcache();
  mc->m = m;
  mc->acked = 0;
  mc->time = get_sim_time();
  bufmsg[nextseqnum] = mc;
  printf("[A] get msg!");

  if (nextseqnum < base + N) {
    struct pkt* packet = makePacket(nextseqnum, acknum[0], bufmsg[nextseqnum]->m->data);
    sendPacket(0, packet);
  }
  nextseqnum++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
  if (packet.checksum != checkSum(&packet) || checkLen(packet.payload) == 0) {
    printf("[A] bad packet!\n");
    return;
  }
  if(packet.acknum < base || packet.acknum > min(base + N, nextseqnum)){
    printf("[A] out of order!\n");
    return;
  }
  bufmsg[packet.acknum]->acked = 1;
  if(packet.acknum == base){
    base = packet.acknum + 1;
  }
  acknum[0] = packet.seqnum;
  printf("[A - getACK] %d,%d,%.20s\n",packet.seqnum, packet.acknum, packet.payload);
}

/* called when A's timer goes off */
void A_timerinterrupt() {
  int curtime = get_sim_time();
  for (int i = base; i < min(base + N, nextseqnum); i++) {
    if(bufmsg[i]->acked == 0 && bufmsg[i]->time + T_INTERVAL < curtime){
      printf("[A - resend]");
      struct pkt* packet = makePacket(i, acknum[0], bufmsg[i]->m->data);
      sendPacket(0, packet);
      bufmsg[i]->time = curtime;
    }
  }
  starttimer(0, CHECK_INTERVAL);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
  seqnum[0] = 1;
  acknum[0] = 1;

  nextseqnum = 1;
  base = 1;
  N = getwinsize();

  starttimer(0, CHECK_INTERVAL);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
  if (packet.checksum != checkSum(&packet) || checkLen(packet.payload) == 0) {
    printf("[B] bad packet!\n");
    return;
  }
  if(packet.seqnum < baseB - N || packet.seqnum > baseB + N){
    printf("[B] %d - %d | out of order!\n", packet.seqnum, baseB);
    return;
  }
  char message[20];
  strncpy(message, packet.payload, 20);

  struct msg* m = new msg();
  strncpy(m->data, message, 20);
  bufdata[packet.seqnum] = m;

  if(baseB == packet.seqnum){
    while(bufdata[baseB] != NULL){
      strncpy(message, bufdata[baseB]->data, 20);
      printf("[B - in] %d|%d,%d,%.20s\n", baseB, packet.seqnum, packet.acknum, message);
      tolayer5(1, message);
      baseB++;
    }
  }
  // acknum[1] = packet.seqnum;
  // seqnum[1] = packet.acknum + 1;
  struct pkt *ack = makePacket(packet.acknum + 1, packet.seqnum, message);
  sendPacket(1, ack);

}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(){
  memset(bufdata, 0, sizeof(int)*1001);
  baseB = 1;
}
