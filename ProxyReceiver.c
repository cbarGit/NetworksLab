/* ProxyReceiver.c */
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
#include <time.h>/* general headers */

#include "Util.h"/* project header */

int h;
/* Pointers for packages' list */
struct PackList *sentinel_PRINT_list;
struct PackList *sentinel_list;
struct PackList *sentinel_SAVE_list;


int totSize = 0;
int printed=0;
void sig_print(int signo){
	
	if(printed==0)
	{
		printed=1;

		if(signo==SIGINT)		printf("\t\n%sSIGINT from keyboard\n%stotSize: %s%i\n%s", RED, WHITE,  RED, totSize, WHITE);
		else if(signo==SIGHUP)	printf("SIGHUP\n");
		else if(signo==SIGTERM)	printf("SIGTERM\n");
		else					printf("other signo\n");
		printf("\n");
		
		fflush(stdout);
	}
	exit(0);
	return;
}

void usage(void) {
	printf ("usage: ./ProxyReceiver PROXYRECEIVERIP fromRITARDATOREPORT RECEIVERIP toRECEIVERPORT"
	"example: ./ProxyReceiver 127.0.0.1 63000 127.0.0.1 64000\n" );
}

int main(int argc, char *argv[]){
	
	/* *********** declarations *********** */
	uint32_t sockfd, recvSize, ris, risTCP, optval, nready;/* forsockets */
	uint32_t c = 1;/* ID counter */
    uint16_t portnoREC, portnoRit;/* for sockets */
    int TCPsockfd;/* for sockets */
    int addr_size;
    int a = 0;/* Number of received packages */
	int rem = 0;/* Number of removed packages from list */
	int go = 0;/* variable for block the 'hurryup' package at the beginning of the connection*/
	int R = 0;/* Partial amount receive from UDP */
	int icmp = 0; /* Icmp packages received from Ritardatore */
	int inviato = 0;/* ID of the last package sent */
	int block = 0;/* variable for block the 'Closing strategy' */
	int list = 0;/* Actual number of item in the list */
	int time = 0;/* To start to calculate the elapsed time */
	/* for the decide if ports are free or closed */
	int free_port0 = 0;
	int free_port1 = 0;
	int free_port2 = 0;
	double tclock1, CLOCK, H, START, END; /* time calculation's variables */
    struct sockaddr_in To, Local, serv_addr;/* for the socket */
    struct timeval timer;
	/* for manage the creation of a package */
	BODY Pack;
	BODY *p = &Pack;
    char buf[BODYSIZE];/* UDP package buffer */ 
    /* for the socket */
    char RitIP[ADDRESSSTRINGLEN];
    char RecIP[ADDRESSSTRINGLEN];
    fd_set rset, allset;
    socklen_t clilen;
    
    if (argc < 2){
		strcpy( RitIP, "127.0.0.1" );
		strcpy( RecIP, "127.0.0.1" );
		portnoRit = 63000;
		portnoREC = 64000;
		printf("\nDefault params: \nRit addr: 127.0.0.1\nport to Rit: 63000\nREC addr: 127.0.0.1\nport to REC: 64000\n\n");
	}
    else if (argc < 3){
		strcpy( RitIP, argv[1] );
		portnoRit = 63000;
		strcpy( RecIP, "127.0.0.1" );
		portnoREC = 64000;
		printf("\nRit addr chosen: %s\nport to Rit: 63000\nREC addr: 127.0.0.1\nport to REC: 64000\n\n", RitIP);
    }
    else if (argc < 4){
		strcpy( RitIP, argv[1] );
		portnoRit = atoi( argv[2] );
		strcpy( RecIP, "127.0.0.1" );
		portnoREC = 64000;
		printf("\nRit addr chosen: %s\nport to Rit chosen: %i\nREC addr: 127.0.0.1\nport to REC: 64000\n\n", RitIP, portnoRit);
	}
    else if (argc < 5){
		strcpy( RitIP, argv[1] );
		portnoRit = atoi( argv[2] );
		strcpy( RecIP, argv[3] );
		portnoREC = 64000;
		printf("\nRit addr chosen: %s\nport to Rit chosen: %i\nREC addr chosen: %s\nport to REC: 64000\n\n", RitIP, portnoRit, RecIP);
	}
    else if (argc < 6){
		strcpy( RitIP, argv[1] );
		portnoRit = atoi( argv[2] );
		strcpy( RecIP, argv[3] );
		portnoREC = atoi( argv[4] );
		printf("\nRit addr chosen: %s\nport to Rit chosen: %i\nREC addr chosen: %s\nport to REC chosen: %i\n\n", RitIP, portnoRit, RecIP, portnoREC);
	}
	else if (argc > 5) { 
		printf ("Wrong arguments\n"); 
		usage(); 
		exit(1);
	}
    
    if ((signal (SIGHUP, sig_print)) == SIG_ERR) { perror("%ssignal (SIGHUP) failed: %s"); exit(2); }
	if ((signal (SIGINT, sig_print)) == SIG_ERR) { perror("%ssignal (SIGINT) failed: %s"); exit(2); }
	if ((signal (SIGTERM, sig_print)) == SIG_ERR) { perror("%ssignal (SIGTERM) failed: %s"); exit(2); }
    
	printf("let's go!\n\n");
    /* First call to socket() function */
    printf("First: initialize the UDP socket\n");
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket\n");
        exit(1);
    }
    
    optval = 1;
	printf ("then set the UDP socket option: REUSEADDR\n");
	ris = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));
	if (ris < 0){
		printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
		}
    
    /* ********* UDP *********** */
    addr_size = sizeof(struct sockaddr_in);
    
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portnoRit);
    printf("bind UDP socket to the server..\n");
    
	ris = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if ( ris < 0)
    {
         printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
         exit(1);
    }
	printf("..done\nincoming UDP connection..\n");
	printf("..from %s : %d..\n\n", RitIP, ntohs(serv_addr.sin_port));
	
	/* ************ TCP ****************** */
	
	TCPsockfd = socket(AF_INET,SOCK_STREAM,0);

	if (TCPsockfd < 0) 
    {
        perror("ERROR opening socket\n");
        exit(1);
    }
	
	optval = 1;
	printf ("then set the TCP socket option: REUSEADDR\n");
	risTCP = setsockopt(TCPsockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));
	if (risTCP < 0){
		printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
		}
	
	memset ( &Local, 0, sizeof(Local) );
	Local.sin_family		=	AF_INET;
	Local.sin_addr.s_addr	=	inet_addr(RecIP);        
	Local.sin_port	=	htons(portnoREC);
	
	risTCP = bind(TCPsockfd, (struct sockaddr*) &Local, sizeof(Local));
	if (risTCP < 0)  {
		printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
		exit(1);
	}
	
	memset( &To,0,sizeof(To));
	To.sin_family		=	AF_INET;
	To.sin_addr.s_addr  =	inet_addr(RecIP);
	To.sin_port			=	htons(portnoREC);
	
	if (connect(TCPsockfd,(struct sockaddr *)&To, sizeof(To))<0){
		printf("Connection Failed, Err: %d %s\n",errno,strerror(errno));
		exit(1);
	}else{
		printf("Connected to: %s : %d\n", inet_ntoa(To.sin_addr), ntohs(To.sin_port) );
	}
	
    
    /* configuring Select() */
	FD_ZERO(&allset);
	FD_SET(sockfd, &allset);
    printf("---------------\n\n");
	printf("waiting for packages..\n");

	
    while (1) {/* once here this will never stop */
		double tclock2;
		struct timeval timeout;
		
		timeout.tv_sec = 0;		
		timeout.tv_usec = TIMEOUT;/* setting timeout for select */
		rset = allset;	
		
		
		
		nready = select(sockfd+1,&rset,NULL,NULL,&timeout);
		while (nready < 0 && errno==EINTR){/* handling error */
			if(nready<0)perror("Select error ");
			exit(1);
			}
		
		if (FD_ISSET(sockfd,&rset) ){/* from Ritardatore */
			
			BODY* pkt = NULL;
		
			go = 1;	/* allow sending 'hurryups' */		
			
			clilen = sizeof(serv_addr);
			recvSize = recvfrom(sockfd, buf, BODYSIZE, 0,(struct sockaddr *)&serv_addr, &clilen);

			if ( recvSize < 0) {/* handle error */
				if (errno != EINTR){
					printf ("recvfrom() failed, Errno: %d \"%s\"\n",
							errno,
							strerror(errno)
						   );
					exit(1);
				}
			}
			
			/* if ICMP arrived close the port (of Ritardatore, so don't send anything to that port) from where you received the package. */
			if( ((ICMP *)buf)->tipo == 'I')
				{
					icmp++;
					if ( ntohs(serv_addr.sin_port) == 62000)free_port0 = 1;
					if ( ntohs(serv_addr.sin_port) == 62001)free_port1 = 1;
					if ( ntohs(serv_addr.sin_port) == 62002)free_port2 = 1;
			}else{	
			if ( ntohs(serv_addr.sin_port) == 62000)free_port0 = 0;
			if ( ntohs(serv_addr.sin_port) == 62001)free_port1 = 0;
			if ( ntohs(serv_addr.sin_port) == 62002)free_port2 = 0;
			}
			
			if (free_port0 == 1 && free_port1 == 1 && free_port2 == 1){/* correct the worst case...if all ports(of Ritardatore) are closed, set'em free */
				free_port0 = 0;
				free_port1 = 0;
				free_port2 = 0;
			}
			
			if(recvSize > 0 && time == 0){/* start timer */
				gettimeofday(&timer,NULL);
				START=timer.tv_sec+(timer.tv_usec/1000000.0);
				time++;
			}
			
			/* timer for sending 'hurryup' */
			gettimeofday(&timer, NULL);
			tclock1=timer.tv_sec+(timer.tv_usec/1000000.0);
			
			R += recvSize; /* encrease partial amount receive from UDP */
			a += 1;/* encrease number of Rx Packages */
			
			if (recvSize == 0){
				printf("end connection.\n");
			}
			if (recvSize < 0){
				perror("ERROR on accept\n");
				}
			
			/* for handling BODY packages received from Ritardatore */
			if( ((BODY *)buf)->tipo == 'B' && block == 0)
					{ if ( ntohl(((BODY *)buf)->id_network_order)>inviato ){/* if ID's pckage is more then the one's sent, save the package and sent an acknolesgement back */
						 ACK* ack;
						 pkt = save(p, (BODY *)buf)->Pack;/* create package in the list */
						 list++;/* amcrease number of package in the list*/
						 memcpy((BODY *)pkt, (BODY *)buf, BODYSIZE);/* copy BODY data in the package */
						 ack = (ACK*)malloc(sizeof(ACK));
						 if (ack==NULL){perror("error: ");exit(1);}	
						 ack->id_network_order=((BODY *)buf)->id_network_order;/* create ACK package to send back, this fill the 'id_network_order' field */
						 ack->id_received=((BODY *)buf)->id_network_order;/* this fill the 'id_received' field */
						 ack->tipo='B';/* set 'B' field to make it pass throught Ritardatore  */
						 ack->ack='A';/* set 'A' to establish that it's an Acknolegment package */
						 send_udp(sockfd, (char*)ack, sizeof(ACK), addr_size, RitIP, free_port0, free_port1, free_port2);/* send the package back */
						 free(ack);/* free memory (malloc) used */
						 }
					}
			
			if( ((ACK *)buf)->ack == 'S' ){/* package of the 'Exit routine strategy'..so print time and exit */
				printf("\n\n%s\"EXIT routine confirmed\" package received\n\t\tEXIT!!%s\n\n", RED,WHITE);
				END=CLOCK-START;
				printf("Time elapsed: %s%3.2f seconds%s\n\n", LBLUE, END, WHITE);
				exit(1);
				}
			
		}/* ISSET */
		
		/* scan the list: 1) for sending 'Exit strategy' package back 2) for sending packages to Receiver with TCP protocol and remove */
		sentinel_list=start;
		while(sentinel_list!=NULL && block == 0)
		{	
			if(ntohl(((BODY *)sentinel_list->Pack)->id_network_order) == c &&/* 'Exit strategy' case */
			   ((BODY *)sentinel_list->Pack)->stop == 'F' )
				{
				ACK* ack;	
				printf("\n\nPartial amount receive from UDP:%s%i%s ", LBLUE, R, WHITE);
				printf("\nPartial amount sent to TCP:%s%i%s ", LBLUE, totSize, WHITE);
				printf("\nNumber of Rx Packages:%s%i%s ", RED, a, WHITE);
				printf("\nNumber of ICMP Rx Packages:%s%i%s ", RED, icmp, WHITE);
				printf("\nNumber of Tx Packages:%s%i%s ", GREEN, c-1, WHITE);
				printf("\nEnd of data at pack number %s%i%s ", YELLOW, c-1, WHITE);
				fflush(stdout);
				removeList(sentinel_list);
				list--;
				rem += 1;
				printf("\rNumber of removed from list: %s%i%s ", RED, rem, WHITE);
				fflush(stdout);
				close(TCPsockfd);
				ack = (ACK*)malloc(sizeof(ACK));
				if (ack==NULL){perror("error: ");exit(1);}		
				ack->id_network_order=((BODY *)buf)->id_network_order;
				ack->id_received=((BODY *)buf)->id_network_order;
				ack->tipo='B';
				ack->ack='F';
				send_udp(sockfd, (char*)ack, sizeof(ACK), addr_size, RitIP, free_port0, free_port1, free_port2);
				free(ack);/* free memory (malloc)*/
				block++;
				gettimeofday(&timer,NULL);
				CLOCK=timer.tv_sec+(timer.tv_usec/1000000.0);
				}/* first if */
				
			if( ntohl(((BODY *)sentinel_list->Pack)->id_network_order) == c &&/* TCP to Receiver case */
				((BODY *)sentinel_list->Pack)->stop != 'F' &&
				block == 0 ) 
			    {
				ACK* ack;
				send_tcp(TCPsockfd, ((BODY *)sentinel_list->Pack)->body, ntohl(((BODY *)sentinel_list->Pack)->nSize));/* send to Receiver throught TCP */
				/* create 'T' package */
				ack = (ACK*)malloc(sizeof(ACK));
				if (ack==NULL){perror("error: ");exit(1);}			
				ack->id_network_order=htonl(c);
				ack->id_received=htonl(c);
				ack->tipo='B';
				ack->ack='T';
				send_udp(sockfd, (char*)ack, sizeof(ACK), addr_size, RitIP, free_port0, free_port1, free_port2);/* send 'T' package */
				free(ack);/* free memory (malloc)*/
				removeList(sentinel_list);/* remove from list */
				rem += 1;
				list--;
				inviato = c;
				c += 1;
				printf("\n\nNumber of Rx Packages:%s%i%s\nNumber of ICMP Rx Packages:%s%i%s\nNumber of Tx Packages:%s%i%s\nNumber of item in the list: %s%i%s\nNumber of removed from list: %s%i%s\r", RED, a, WHITE, RED, icmp, WHITE, GREEN, c-1, WHITE, RED, list, WHITE, RED, rem, WHITE);
				fflush(stdout);
				/*refresh last package time*/ 
				gettimeofday(&timer,NULL);
				CLOCK=timer.tv_sec+(timer.tv_usec/1000000.0);
				}/* second if */
			
			sentinel_list=sentinel_list->next;
			if (sentinel_list==NULL){
					break;
					}		
		}/* WHILE sentinel_list */
        
        /* timer from last 'hurryup' sent */
        gettimeofday(&timer, NULL);
		tclock2=timer.tv_sec+(timer.tv_usec/1000000.0);
        H = tclock2-CLOCK;
        /* */
		if ( H > TIMER && block == 0 && go > 0 )/* send 'hurryup' package */
			{	
				ACK* ack;
				ack = (ACK*)malloc(sizeof(ACK));
				if (ack==NULL){perror("error: ");exit(1);}			
				ack->id_network_order=htonl(c);
				ack->id_received=htonl(c);
				ack->tipo='B';
				ack->ack='H';
				send_udp(sockfd, (char*)ack, sizeof(ACK), addr_size, RitIP, free_port0, free_port1, free_port2);
				free(ack);
				gettimeofday(&timer, NULL);
				CLOCK=timer.tv_sec+(timer.tv_usec/1000000.0);
				H = 0;
			}
		
		
		if(block>0){/* closing strategy */
			double tclock2;
			ACK* ack;
			/* if after 5 seconds it doesn't receive 'Exit package'..close everything, send 'Exit package' back and exit */
			gettimeofday(&timer, NULL);
			tclock2=timer.tv_sec+(timer.tv_usec/1000000.0);
			H = tclock2-tclock1;		
			if(H > 5){
			printf("\n\n%sToo much time elapsed since last package received\nCLOSE TCP CONNECTION and EXIT%s\n\n", RED, WHITE);
			printf("\n\n%s\"EXIT routine confirmed\" package received\n\t\tEXIT!!%s\n\n", RED,WHITE);
			END=CLOCK-START;
			printf("Time elapsed: %s%3.2f seconds%s\n\n", LBLUE, END, WHITE);
			close(TCPsockfd);
			ack = (ACK*)malloc(sizeof(ACK));
			if (ack==NULL){perror("error: ");exit(1);}			
			ack->id_network_order=htonl(c);
			ack->id_received=htonl(c);
			ack->tipo='B';
			ack->ack='F';
			send_udp(sockfd, (char*)ack, sizeof(ACK), addr_size, RitIP, free_port0, free_port1, free_port2);				
			send_udp(sockfd, (char*)ack, sizeof(ACK), addr_size, RitIP, free_port0, free_port1, free_port2);				
			send_udp(sockfd, (char*)ack, sizeof(ACK), addr_size, RitIP, free_port0, free_port1, free_port2);				
			free(ack);
			exit(1);
			}
		}
		
		
}/* WHILE(1)*//* you'll never pass throught this */

return 0;

}/* main */
