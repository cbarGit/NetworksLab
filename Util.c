/* Util.c */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#include "Util.h"

/*int h;
int hRem = 0;*/
int totSize;
int r;
double t1;
struct timeval timer;
struct PackList *start = NULL;
struct PackList *end = NULL;
struct PackList *current;
struct PackList *currentRem;
struct PackList *temp;
struct PackList *tempRem;

struct PackList *sentinel_PRINT_list;
struct PackList *sentinel_list;
struct PackList *sentinel_SAVE_list;


struct PackList * createpack(){/* Function for create an UDP package */
	struct PackList * new=(struct PackList *)malloc(sizeof(struct PackList));
	if (new == NULL){perror("error: ");exit(1);}
	new->Pack=(BODY *)malloc(sizeof(BODY));
	if (new->Pack == NULL){perror("error: ");exit(1);}
	new->next=NULL;
	new->prev=NULL;
	return new;
}


struct PackList* insertBefore(struct PackList *new, struct PackList *sentinel_SAVE_list){/* function to insert a package before another one */
	
	if(sentinel_SAVE_list->prev == NULL ){
		start=new;
	}
	else{
		(sentinel_SAVE_list->prev)->next=new;
	}
	new->prev=sentinel_SAVE_list->prev;
	new->next=sentinel_SAVE_list;
	sentinel_SAVE_list->prev=new;
	
	
	return new;
}

struct PackList * insertAfter(struct PackList *new, struct PackList *sentinel_SAVE_list){/* function to insert a package after another one */

	if (sentinel_SAVE_list->next == NULL){
		end=new;
	}
	else{
		(sentinel_SAVE_list->next)->prev=new;
	}
	new->prev = sentinel_SAVE_list;
	new->next = sentinel_SAVE_list->next;
	sentinel_SAVE_list->next = new;
	
	
	return new;

}

struct PackList * save_for_sender(BODY *p){/* function used by ProxySender for create UDP packages from TCP stream and saving them in a list without order (the order is given by 'ID' added gradually in the package */
	struct PackList *new=createpack();
	if(start==NULL){
		start=new;
		end=new;
		new->next=NULL;
		new->prev=NULL;
		} else {
			end->next=new;
			new->prev=end;
			new->next=NULL;
			end=new;
			}
		return new; 	
}

struct PackList * save(BODY *p, BODY *buf){/* function used by ProxyReceiver for saving UDP packages received from Ritardatore and order them in a list */
	struct PackList *new=createpack();
	if(start==NULL){
		start=new;
		end=new;
		new->next=NULL;
		new->prev=NULL;
		} else {
			sentinel_SAVE_list=start;
			while(sentinel_SAVE_list!=NULL){
				if( ((BODY*)buf)->id_network_order > ((BODY*)sentinel_SAVE_list->Pack)->id_network_order && sentinel_SAVE_list->next!=NULL){
				sentinel_SAVE_list=sentinel_SAVE_list->next;
				}
				else if( ((BODY*)buf)->id_network_order < ((BODY*)sentinel_SAVE_list->Pack)->id_network_order){
					insertBefore(new,sentinel_SAVE_list);
					break;
				}
				else if((((BODY*)buf)->id_network_order > ((BODY*)sentinel_SAVE_list->Pack)->id_network_order) && sentinel_SAVE_list->next==NULL){
					insertAfter(new,sentinel_SAVE_list);
					break;
					}
			}
		}
		return new; 	
}

void removeList(struct PackList *sentinel_list){/* function used by ProxySender and ProxyReceiver to remove UDP packages from the list */
		if(sentinel_list->prev==NULL){
			start = sentinel_list->next;
		}else{
			(sentinel_list->prev)->next = sentinel_list->next;
		}
		if(sentinel_list->next==NULL){
			end=sentinel_list->prev;
		}else{
			(sentinel_list->next)->prev = sentinel_list->prev;
		}
		free(sentinel_list);
}



int send_tcp(uint32_t socketfd, char *buf, uint32_t size){/* function used by ProxyReceiver to send TCP stream to Receiver */
	int ris;
 
	ris=send(socketfd, buf, size, 0);
	totSize+= size;
	
	if (ris < 0) {
		printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
		return(0);
	}
	return(1);
}


int send_udp_Sender(uint32_t socketfd, char* buf, uint32_t size, char *RitIP, int free_port0, int free_port1, int free_port2){/* function used by ProxySender to send UDP packages to Ritardatore based on open doors */
	int addr_size = sizeof(struct sockaddr_in);
	int ris;
	struct sockaddr_in UDPsend;
	int port[3] = {61000, 61001, 61002};
	
	
	memset(&UDPsend, 0, sizeof(UDPsend));

    UDPsend.sin_family = AF_INET;
    UDPsend.sin_addr.s_addr = inet_addr(RitIP);

	if ( free_port0 == 0 && free_port1 == 0 && free_port2 == 0 ){
		r = (r + 1) % 3;
		UDPsend.sin_port = htons(port[r]);
	}
	if ( free_port0 == 0 && free_port1 == 1 && free_port2 == 0 ){
		int available[2] = {61000, 61002};
		r = (r + 1) % 2;
		UDPsend.sin_port = htons(available[r]);
	}
	if ( free_port0 == 1 && free_port1 == 0 && free_port2 == 0 ){
		int available[2] = {61001, 61002};
		r = (r + 1) % 2;
		UDPsend.sin_port = htons(available[r]);
	}
	if ( free_port0 == 0 && free_port1 == 0 && free_port2 == 1 ){
		int available[2] = {61000, 61001};
		r = (r + 1) % 2;
		UDPsend.sin_port = htons(available[r]);
	}
	if ( free_port0 == 0 && free_port1 == 1 && free_port2 == 1 ){
		UDPsend.sin_port = htons(61000);
	}
	if ( free_port0 == 1 && free_port1 == 0 && free_port2 == 1 ){
		UDPsend.sin_port = htons(61001);
	}
	if ( free_port0 == 1 && free_port1 == 1 && free_port2 == 0 ){
		UDPsend.sin_port = htons(61002);
	}
	
	/*send to the address*/
	ris = sendto(socketfd, buf, size, 0,(struct sockaddr*)&UDPsend, addr_size);
	/*printf("ris = %i\n", ris);*/
	if (ris < 0) {
		printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
		return(0);
	}
	return(1);
}


int send_udp(uint32_t socketfd, char* buf, uint32_t size, int addr_size, char *RitIP, int free_port0, int free_port1, int free_port2){/* function used by ProxyReceiver to send UDP packages to Ritardatore based on open doors */
	int ris;
	struct sockaddr_in UDPsend;
	int port[3] = {62000, 62001, 62002};
	
	memset(&UDPsend, 0, sizeof(UDPsend));

    UDPsend.sin_family = AF_INET;
    UDPsend.sin_addr.s_addr = inet_addr(RitIP);
    if ( free_port0 == 0 && free_port1 == 0 && free_port2 == 0 ){
		r = (r + 1) % 3;
		UDPsend.sin_port = htons(port[r]);
	}
	if ( free_port0 == 0 && free_port1 == 1 && free_port2 == 0 ){
		int available[2] = {62000, 62002};
		r = (r + 1) % 2;
		UDPsend.sin_port = htons(available[r]);
	}
	if ( free_port0 == 1 && free_port1 == 0 && free_port2 == 0 ){
		int available[2] = {62001, 62002};
		r = (r + 1) % 2;
		UDPsend.sin_port = htons(available[r]);
	}
	if ( free_port0 == 0 && free_port1 == 0 && free_port2 == 1 ){
		int available[2] = {62000, 62001};
		r = (r + 1) % 2;
		UDPsend.sin_port = htons(available[r]);
	}
	if ( free_port0 == 0 && free_port1 == 1 && free_port2 == 1 ){
		UDPsend.sin_port = htons(port[0]);
	}
	if ( free_port0 == 1 && free_port1 == 0 && free_port2 == 1 ){
		UDPsend.sin_port = htons(port[1]);
	}
	if ( free_port0 == 1 && free_port1 == 1 && free_port2 == 0 ){
		UDPsend.sin_port = htons(port[2]);
	}
    		
	/*send to the address*/
	ris = sendto(socketfd, buf, size, 0,(struct sockaddr*)&UDPsend, addr_size);
	
	if (ris < 0) {
		printf ("sendto() failed, Error: %d \"%s\"\n", errno,strerror(errno));
		return(0);
	}
	
	return(1);
}
