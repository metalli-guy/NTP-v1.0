#include "includesdefinesvariables.h"
#include "mbed.h"
Timer offsettimer;
EthernetInterface net;
typedef struct {
  uint8_t li_vn_mode;
  uint8_t stratum;
  uint8_t poll;
  uint8_t precision;
  uint32_t rootDelay;
  uint32_t rootDispersion;
  uint32_t refId;
  uint32_t refTm_s;
  uint32_t refTm_f;
  uint32_t origTm_s;
  uint32_t origTm_f;
  uint32_t rxTm_s;
  uint32_t rxTm_f;
  uint32_t txTm_s;
  uint32_t txTm_f;
} ntp_packet;
int NTP_gettime() {
  // defining packet to be sent, imperative that we send time(NULL) for txTM_s,
  // lets us calculate offset and delay
  ntp_packet packet = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, time(NULL), 0};
  //
  memset(&packet, 0, sizeof(ntp_packet));
  //
  // li = 0, vn = 3, and mode = 3
  *((char *)&packet + 0) = 0x1b;
  //
  // opening sockets and connecting to static ip & NTP server
  SocketAddress sockAddr;
  int i = net.set_network(IP, MASK, GATEWAY);
  i = net.connect();
  net.get_ip_address(&sockAddr);
  UDPSocket sock;
  sock.open(&net);
  net.gethostbyname("0.pool.ntp.org", &sockAddr);
  sockAddr.set_port(123);
  // sending & receiving data, defining timer for packet to depart and return
  offsettimer.start();
  if (0 > sock.sendto(sockAddr, (char *)&packet, sizeof(ntp_packet))) {
    printf("Error sending data");
    return -1;
  }
  sock.recvfrom(&sockAddr, (char *)&packet, sizeof(ntp_packet));
  offsettimer.stop();
  uint32_t timerduration = offsettimer.read_ms();
  offsettimer.reset();
  //
  // converting network byte order to host byte order
  packet.origTm_s = ntohl(packet.origTm_s);
  packet.origTm_f = ntohl(packet.origTm_f);
  packet.txTm_s = ntohl(packet.txTm_s);
  packet.txTm_f = ntohl(packet.txTm_f);
  packet.rxTm_s = ntohl(packet.rxTm_s);
  packet.rxTm_f = ntohl(packet.rxTm_f);
  //
  // defining variables for offset & delay calculation
  uint32_t destTm_s = (time(NULL));
  uint32_t t1 = packet.origTm_s;
  uint32_t t2 = packet.rxTm_s;
  uint32_t t3 = packet.txTm_s;
  uint32_t t4 = destTm_s;
  uint32_t t5 = packet.origTm_f;
  uint32_t t6 = packet.rxTm_f;
  uint32_t t7 = packet.txTm_f;
  uint32_t t8 = timerduration; // desttmF
  t8 = t8 / MILLIS_CONVERSION_CONSTANT;
  //
  // calculation for second-based delay
  uint64_t offset =
      (packet.rxTm_s - packet.origTm_s) + (packet.txTm_s - destTm_s) / 2;
  uint64_t delay =
      (destTm_s - packet.origTm_s) - (packet.txTm_s - packet.rxTm_s);
  uint64_t totalOffset = offset + delay;
  totalOffset = (totalOffset / 4294967.296);
  //
  // fractional calculation for sleeping
  uint64_t offset_f =
      (packet.rxTm_f - packet.origTm_f) + (packet.txTm_f - t8) / 2;
  uint64_t delay_f = (t8 - packet.origTm_f) - (packet.txTm_f - packet.rxTm_f);
  offset_f = offset_f * MILLIS_CONVERSION_CONSTANT;
  delay_f = delay_f * MILLIS_CONVERSION_CONSTANT;
  packet.txTm_f = packet.txTm_f * MILLIS_CONVERSION_CONSTANT;
  uint64_t totalOffset_f = offset_f + delay_f;
  //
  // defining the waiting time for PPS start, defining sleep
  uint32_t waittimer = (packet.txTm_f + totalOffset_f);
  uint32_t waittimer_f = waittimer % 1000;
  uint32_t waittimer_s = waittimer / 1000;
  uint32_t nextSecondTime = 1000 - waittimer_f;
  uint32_t sleepduration = (1000 - nextSecondTime);
  //
  // calculating exact timestamp to be written into RTC
  time_t txTm = packet.txTm_s - NTP_TIMESTAMP_DELTA - (totalOffset / 1000) +
                waittimer_s;
  //
  // defining buffer, printing NTP time, setting time to RTC
  char buffer[32];
  ThisThread::sleep_for(sleepduration);
  strftime(buffer, 32, "%a,%d %B %Y %H:%M:%S", localtime(&txTm));
  nextSecondTime = 000;
  printf("NTP Time = %s:%u\n", buffer, nextSecondTime);
  set_time(txTm);
  // closing the socket and disconnecting till future use
  sock.close();
  net.disconnect();
  return 0;
}