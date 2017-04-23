#define MAX_NUM_IFACES       100

typedef struct {
	int   sd;
	char *ifname;
} ifsock_t;

void if_init (char *iface[], int num);
int  if_exit (void);

void if_send (uint8_t interval);
void if_poll (uint8_t interval);
