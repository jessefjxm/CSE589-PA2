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

int seqnum[2];
int acknum[2];
struct msg* bufmsg[1001];

int nextseqnum, base, N;

int baseB;

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
  bufmsg[nextseqnum] = m;
  printf("[A] get msg!");

  if (nextseqnum < base + N) {
    struct pkt* packet = makePacket(nextseqnum, acknum[0], bufmsg[nextseqnum]->data);
    sendPacket(0, packet);
    if (base == nextseqnum) {
      starttimer(0, 40);
    }
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
  base = packet.acknum + 1;
  acknum[0] = packet.seqnum;
  stoptimer(0);
  printf("[A - getACK] %d,%d,%.20s\n",packet.seqnum, packet.acknum, packet.payload);
}

/* called when A's timer goes off */
void A_timerinterrupt() {
  printf("[A - resend]");
  for (int i = base; i < min(base + N, nextseqnum); i++) {
    struct pkt* packet = makePacket(i, acknum[0], bufmsg[i]->data);
    sendPacket(0, packet);
  }
  starttimer(0, 40);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
  seqnum[0] = 1;
  acknum[0] = 1;

  nextseqnum = 1;
  base = 1;
  N = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
  if (packet.checksum != checkSum(&packet) || checkLen(packet.payload) == 0) {
    printf("[B] bad packet!\n");
    return;
  }
  if (packet.seqnum != baseB) {
    printf("[B] gap!\n");
    return;
  }
  char message[20];
  strncpy(message, packet.payload, 20);

  acknum[1] = packet.seqnum;
  seqnum[1] = packet.acknum + 1;
  struct pkt *ack = makePacket(seqnum[1], acknum[1], message);
  sendPacket(1, ack);

  printf("[B - in] %d|%d,%d,%.20s\n", baseB, packet.seqnum,
         packet.acknum, message);
  tolayer5(1, message);
  baseB++;
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(){
    seqnum[1] = 1;
    acknum[1] = 1;
    baseB = 1;
}
