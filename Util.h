/* colors for screen outputs macros */
#define WHITE "\033[0m"
#define RED  "\033[22;31m"
#define GREEN  "\033[22;32m"
#define YELLOW  "\033[22;33m"
#define BLUE  "\033[22;34m"
#define LBLUE  "\033[22;36m"
#define PURPLE  "\033[22;35m"

/* macros for: */
#define BUFFER 64000 /* size of body field inside the package */
#define BODYSIZE (BUFFER)+10 /* size of package (header included) */
#define ADDRESSSTRINGLEN 32 /* length for string (for general uses) */ 
#define TIMEOUT 80000/* Select timeout */
#define TIMER 0.05 /* timeout to send hurryup to ProxySender */

int hRem;
struct PackList *start;
struct PackList *end;

struct timeval timer;

/* UDP package list structure */
struct PackList{
	void *Pack;/* pointer to real package */
	struct PackList *next;/* pointer to next in the list */
	struct PackList *prev;/* pointer to prev in the list */
} __attribute__((packed));

/* ICMP package structure */
typedef struct {
	uint32_t id_network_order;
	char tipo;
	uint32_t packet_lost_id_network_order;
}  __attribute__((packed)) ICMP;

/* Package structure */ 
typedef struct {
	uint32_t id_network_order;
	char tipo;
	char stop; /* field for last package of file */
	uint32_t nSize; /* field for the quantity of BUFFER to send */
	char body[BUFFER];/* buffer for data */ 
} __attribute__((packed)) BODY;

/* Acknowledgement package structure */
typedef struct {
	uint32_t id_network_order;
	char tipo;
	char ack;
	uint32_t id_received;
} __attribute__((packed)) ACK;

struct PackList * createpack();
struct PackList * insertBefore(struct PackList *new, struct PackList *sentinel_SAVE_list);
struct PackList * insertAfter(struct PackList *new, struct PackList *sentinel_SAVE_list);
struct PackList * save_for_sender(BODY *p);
struct PackList * save(BODY *p, BODY *buf);
void removeList(struct PackList *sentinel_list);
int send_tcp(uint32_t socketfd, char *buf, uint32_t size);
int send_udp_Sender(uint32_t socketfd, char* buf, uint32_t size, char *RitIP, int free_port0, int free_port1, int free_port2);
int send_udp(uint32_t socketfd, char* buf, uint32_t size, int addr_size, char *RitIP, int free_port0, int free_port1, int free_port2);

