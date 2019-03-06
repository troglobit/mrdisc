#define IGMP_MRDISC_ANNOUNCE 0x30
#define IGMP_MRDISC_SOLICIT  0x31
#define IGMP_MRDISC_TERM     0x32

#define ICMP6_MRDISC_ANNOUNCE	151
#define ICMP6_MRDISC_SOLICIT	152
#define ICMP6_MRDISC_TERM	153

int inet_open   (char *ifname);
int inet6_open  (char *ifname);
int inet_close  (int sd);
int inet6_close (int sd);

int inet_send  (int sd, uint8_t type, uint8_t interval);
int inet6_send (int sd, uint8_t type, uint8_t interval);
int inet_recv  (int sd, uint8_t interval);
int inet6_recv (int sd, uint8_t interval);

