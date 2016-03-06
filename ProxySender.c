/* ProxySender.c */
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

struct timeval timeout;
/* pointers for the list of UDP packages */
struct PackList *current;
struct PackList *currentRem;
struct PackList *temp;
struct PackList *tempRem;
int h;

int printed=0;
void sig_print(int signo){
	
	if(printed==0)
	{
		printed=1;

		if(signo==SIGINT)		printf("\t\n%sSIGINT from keyboard%s\n", RED, WHITE );
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
	printf ("usage: ./ProxySender SENDERIP SENDERPORT PROXYSENDERIP fromRITARDATOREPORT"
	"example: ./ProxySender 127.0.0.1 59000 127.0.0.1 60000\n" );
}

int main(int argc, char *argv[]){
	
	/* *********** Variable declarations *********** */
	uint32_t sockfd, newsockfd, ris, risUDP, optval, maxfd, nready;/* forsockets */
    uint32_t counter=0;/* ID counter */
    uint16_t portnoTCP, portnoUDP; /* for sockets */
    int totN = 0;/* Total bytes read from TCP socket */
    int totNH = 0;/* Total bytes sent to UDP socket */
    int a = 0;/* Total buffer used */ 
    int Ack = 0;/* Total package received from Ritardatore (acks) */
    int icmp = 0;/* Total package received from Ritardatore (acks) */
    int totPacks = 0;/* Total packs RECEIVED from Rit */
    int packTCP = 0;/* TCP packages counter */
    int UDPsockfd;/* UDP socket */
    int free_port0 = 0;/* 0 if port id free 1 if it's closed */
    int free_port1 = 0;
    int free_port2 = 0;
    int hold = 0; /* for control the max number of packages sent to Ritardatore after receiving "hurryup"  */
    char buf[BUFFER];/* buffer for TCP */
    char bufACK[10];/* buffer for UDP */
    char ipaddr[ADDRESSSTRINGLEN];/* address */ 
    char RitIP[ADDRESSSTRINGLEN];/* address */ 
    struct sockaddr_in serv_addr, cli_addr, fromRit;/* socket structs */
    BODY Pack;/* for manage the creation of a package */
    BODY *p = &Pack;/* for manage the creation of a package */
	/* for set up the connections */
    ssize_t n;
    ssize_t m;
    fd_set rset, allset;
    socklen_t clilen;
    socklen_t tosize;
    
    if (argc < 2){
		strcpy( ipaddr, "127.0.0.1" );
		strcpy( RitIP, "127.0.0.1" );
		portnoTCP = 59000;
		portnoUDP = 60000;
		printf("\nDefault params: \nSender addr: 127.0.0.1\nport from TCP: 59000\nRit addr: 127.0.0.1\nport to Rit: 60000\n\n");
	}
    else if (argc < 3){
		strcpy( ipaddr, argv[1] );
		portnoTCP = 59000;
		strcpy( RitIP, "127.0.0.1" );
		printf("\nSender addr chosen: %s\nport from TCP: 59000\nRit addr: 127.0.0.1\nport to Rit: 60000\n\n", ipaddr);
    }
    else if (argc < 4){
		strcpy( ipaddr, argv[1] );
		portnoTCP = atoi( argv[2] );
		strcpy( RitIP, "127.0.0.1" );
		portnoUDP = 60000;
		printf("\nSender addr chosen: %s\nport from TCP chosen: %i\nRit addr: 127.0.0.1\nport to Rit: 60000\n\n", ipaddr, portnoTCP);
	}
    else if (argc < 5){
		strcpy( ipaddr, argv[1] );
		portnoTCP = atoi( argv[2] );
		strcpy( RitIP, argv[3] );
		portnoUDP = 60000;
		printf("\nSender addr chosen: %s\nport from TCP chosen: %i\nRit addr chosen: %s\nport to Rit: 60000\n\n", ipaddr, portnoUDP, RitIP);
	}
    else if (argc < 6){
		strcpy( ipaddr, argv[1] );
		portnoTCP = atoi( argv[2] );
		strcpy( RitIP, argv[3] );
		portnoUDP = atoi( argv[4] );
		printf("\nSender addr chosen: %s\nport from TCP chosen: %i\nRit addr chosen: %s\nport to Rit chosen: %i\n\n", ipaddr, portnoTCP, RitIP, portnoUDP);
    }
    else if (argc > 5) { 
		printf ("Wrong arguments\n"); 
		usage(); 
		exit(1);
	}
    
    
    if ((signal (SIGHUP, sig_print)) == SIG_ERR) { perror("%ssignal (SIGHUP) failed: %s"); exit(2); }
	if ((signal (SIGINT, sig_print)) == SIG_ERR) { perror("%ssignal (SIGINT) failed: %s"); exit(2); }
	if ((signal (SIGTERM, sig_print)) == SIG_ERR) { perror("%ssignal (SIGTERM) failed: %s"); exit(2); }
 
	/* *********** TCP *********** */
	
	printf("let's go!\n\n");

    printf("First: initialize the TCP socket\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket\n");
        exit(1);
    }
    
    optval = 1;
	printf ("then set the TCP socket option: REUSEADDR\n");
	ris = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));
	if (ris < 0){
		printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
		}
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ipaddr);
    serv_addr.sin_port = htons(portnoTCP);
    printf("bind TCP socket to the server and..\n");
	ris = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if ( ris < 0)
    {
         printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
         exit(1);
    }
    
    /* listen from TCP socket, attending data from Sender */
    printf("now listen from TCP socket..\n");
    ris = listen(sockfd,10);
    if (ris < 0)  {
    printf ("listen() failed, Err: %d \"%s\"\n",errno,strerror(errno));
    exit(1);
    }
    
	/* accepting TCP stream */
    printf("---------------\n\n");
    clilen = sizeof(cli_addr);
    memset ( &cli_addr, 0, sizeof(cli_addr) );
	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); 
	if (newsockfd < 0) 
	{
		perror("ERROR on accept\n");
		exit(1);
	}
	printf("incoming TCP connection accepted..\n");
	printf("..from %s\n\n", inet_ntoa(cli_addr.sin_addr));

	printf("to %s : %d\n\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

	/* *********** UDP *********** */
	
    UDPsockfd=socket(AF_INET,SOCK_DGRAM,0);
    if (UDPsockfd < 0) 
    {
        perror("ERROR opening socket\n");
        exit(1);
    }
    
    optval = 1;
	printf ("then set the UDP socket option: REUSEADDR\n");
	risUDP = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));
	if (risUDP < 0){
		printf ("setsockopt() SO_REUSEADDR failed, Err: %d \"%s\"\n", errno,strerror(errno));
		exit(1);
		}

	memset( (char*)&fromRit,0,sizeof(fromRit));
	fromRit.sin_family		=	AF_INET;
	fromRit.sin_addr.s_addr =	htonl(INADDR_ANY);
	fromRit.sin_port		=	htons(portnoUDP);
	
	risUDP = bind(UDPsockfd, (struct sockaddr *) &fromRit, sizeof(fromRit));
  
    if ( risUDP < 0)
    {
         printf ("bind() failed, Err: %d \"%s\"\n",errno,strerror(errno));
         exit(1);
    }
	printf("..done\nincoming UDP connection..\n");
	printf("..from %s : %d..\n\n", RitIP, ntohs(fromRit.sin_port));

    
    /* *********** configuring Select() *********** */
    FD_ZERO(&allset);
    FD_SET(newsockfd, &allset);
	FD_SET(UDPsockfd, &allset);

	
    while (1) {/* once here this will never stop */
		
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = TIMEOUT; /* setting timeout for select */
		
		if(newsockfd>UDPsockfd){/* deciding which socket is ready */
		maxfd = newsockfd;
	}else{
		maxfd = UDPsockfd;
	}
		
		rset = allset;
		
		nready = select(maxfd+1,&rset,NULL,NULL,&timeout);
		if (nready < 0) {/* handling error */
			printf("select() failed, Err: %d \"%s\"\n",
						 errno,
						 strerror(errno)
					  );
			exit(1);
			}
		
		memset(&buf,0,BUFFER);
		
		if (FD_ISSET(newsockfd,&rset) ){/* Accept actual connection from Sender */
			
			BODY* pkt = NULL;
			n = read(newsockfd, buf, BUFFER );	/* read from socket and save data to buffer*/ 
			totN += n;
			
			if ( n < 0 && errno == EINTR ) {/* handle error */
					printf ("read() failed, Errno: %d \"%s\"\n",
							errno,
							strerror(errno)
						   );
					exit(1);
			}
			
			if (n > 0){/* if it si really receiving data */
				printf("READING from TCP socket\n\n");
				pkt = save_for_sender(p)->Pack; /* create and save data in list */
				counter++; /* encrease counter */
				pkt->id_network_order=htonl(counter); /* save it to the package's ID field */
				printf("Counter:..%s%u%s\n", PURPLE, counter, WHITE);
				pkt->tipo='B';/* save 'tipo' field with char 'B' that means that this is a package with data */
				memcpy(pkt->body, buf, n );/* phisically save data read from buffer to 'body' field */
				pkt->nSize = htonl(n);/* save amount of data (just read from socket and saved in 'body') in 'nSize' field of package */  
				packTCP++;/* encrease TCP packages counter */
				printf("nSize..%sdone%s\n", GREEN, WHITE);
				printf("Size of Pack->body: %s%lu%s\n", LBLUE, sizeof(pkt->body), WHITE);
				printf("Package:%s%u%s\n", YELLOW, ntohl(pkt->id_network_order), WHITE);
				a += sizeof(buf); /* encrease amount of buffer used counter */
				send_udp_Sender(UDPsockfd, (char*)pkt, BODYSIZE, RitIP, free_port0, free_port1, free_port2);/* send to Ritardatore the packege just created */
				totNH += BODYSIZE;/* encrease total size of UDP data sent to Ritardatore */
				if(start){/*print list */
				h = 0;
				current=start;
				while(current != NULL){
					current = current->next;
					h += 1;
				}}
				printf("\n%sTotal Pkg Number: %i%s\n\n", GREEN, h, WHITE);
				printf("Total bytes read from TCP socket: %s%i%s\n", BLUE, totN, WHITE);
				printf("Total buffer used: %s%i%s\n", BLUE, a, WHITE);
				printf("Total bytes sent to UDP socket: %s%i%s\n\n\n", BLUE, totNH, WHITE);
			}
	
			if (n == 0){/* if read() is zero means that the file is over and ProxySender needs to create an 'end of file' package to send to ProxyReceiver */
				printf("STILL READING from TCP socket\n\n");
				pkt = save_for_sender(p)->Pack; /* create and save data in list */
				counter++; /* encrease counter */
				pkt->id_network_order=htonl(counter);/* save it to the package's ID field */
				printf("Counter:..%s%u%s\n", PURPLE, counter, WHITE);
				pkt->tipo='B';/* save 'tipo' field with char 'B' that means that this is a package with data (even if has no data) */
				printf("Type: %s%c%s\n", RED, pkt->tipo, WHITE);
				pkt->nSize = htonl(n);/* save amount of data (just read from socket and saved in 'body') in 'nSize' field of package */ 
				printf("nSize..%sdone%s\n", GREEN, WHITE);
				pkt->stop='F';/* save 'stop' field with char 'F' that means that is a package of 'end of file'. It will be used by ProxyReceiver to close connection with everything */ 
				packTCP++;/* encrease TCP packages counter */
				printf("Size of Pack->body: %s%lu%s\n", LBLUE, sizeof(pkt->body), WHITE);
				printf("Package:%s%u%s\n", YELLOW, ntohl(pkt->id_network_order), WHITE);
				a += sizeof(buf);/* encrease amount of buffer used counter */
				send_udp_Sender(UDPsockfd, (char*)pkt, 10, ipaddr, free_port0, free_port1, free_port2);/* send to Ritardatore the packege just created */
				totNH += 10;/* encrease total size of UDP data sent to Ritardatore (this package has only 10 bytes of size)*/
				if(start){/*print list */
				h = 0;
				current=start;
				while(current != NULL){
					current = current->next;
					h += 1;
				}}
				printf("\n%sTotal Pkg Number: %i%s\n\n", GREEN, h, WHITE);
				printf("Total bytes read from TCP socket: %s%i%s\n", BLUE, totN, WHITE);
				printf("Total buffer used: %s%i%s\n", BLUE, a, WHITE);
				printf("Total bytes sent to UDP socket: %s%i%s\n\n\n", BLUE, totNH, WHITE);
				close(newsockfd);/* close TCP connection */
			}
			
		}
		
		
		if (FD_ISSET(UDPsockfd,&rset) ){/* from Ritardatore */
			int hurry;

			memset(&bufACK,0,10);/* clear the buffer */
			tosize = sizeof(fromRit);
			m = recvfrom(UDPsockfd, bufACK, 10, 0, (struct sockaddr *)&fromRit, &tosize);
			
			if ( m < 0 && errno == EINTR) {/* handle error */
					printf ("recvfrom() failed, Errno: %d \"%s\"\n",
							errno,
							strerror(errno)
						   );
					exit(1);
			}
			
			/* set the port from you have just received from Ritardatore as free */
			if ( ntohs(fromRit.sin_port) == 61000)free_port0 = 0;
			if ( ntohs(fromRit.sin_port) == 61001)free_port1 = 0;
			if ( ntohs(fromRit.sin_port) == 61002)free_port2 = 0;
			
			
			/*if (m < 0){printf("NO READING from UDP socket\n\n");
			close(UDPsockfd);};*/
			
			totPacks += 1;/* encrease UDP packages received counter */
			
			/* UDP package's ack from ProxyReceiver ---> package is arrived so remove package from list */
			if( ((ACK *)bufACK)->tipo == 'B' && /* if has 'tipo' as 'B' and 'ack' as 'A'... */
				((ACK *)bufACK)->ack =='A' )
				{
					printf("A-Ack for ID: %s%i%s\n", BLUE,ntohl(((ACK *)bufACK)->id_received),WHITE);
					tempRem=start;
					while( tempRem!=NULL ){/* scan the list 'till it's over */
						if( ntohl(((BODY *)tempRem->Pack)->id_network_order) == /* if the ID of the item of the list is equal to 'id_received'.. */
							ntohl(((ACK *)bufACK)->id_received) )
							{
								printf("Removed id: %u\n", ntohl(((BODY *)tempRem->Pack)->id_network_order));
								removeList(tempRem);/* remove item */
								hRem += 1;/* encrease total remove package counter */
								Ack += 1;/* encrease ack's counter */
								printf("Total removed: %s%i%s\n", GREEN, hRem, WHITE);
								printf("Total ack: %s%i%s\n", GREEN, Ack, WHITE);
								break;
							}
						tempRem=tempRem->next;
						if (tempRem==NULL)break;/* exit from while when at the end of the list */
						}/* WHILE TEMP REM */
				}/* IF */
			
			/* Ack of package just sent to Receiver ---> so remove all the packages with ID equal or less to */
			if( ((ACK *)bufACK)->tipo == 'B' && /* if has 'tipo' as 'B' and 'ack' as 'T'... */
				((ACK *)bufACK)->ack =='T' )
				{
					printf("T-Ack for ID: %s%i%s\n", BLUE,ntohl(((ACK *)bufACK)->id_received),WHITE);
					tempRem=start;
					while( tempRem!=NULL ){/* scan the list 'till it's over */
						if( ntohl(((BODY *)tempRem->Pack)->id_network_order) <= ntohl(((ACK *)bufACK)->id_received) ) /* if ID of the item of the list is equal or less to 'id_received'.. */
							{
								printf("Removed id: %u\n", ntohl(((BODY *)tempRem->Pack)->id_network_order));
								removeList(tempRem);/* remove item */
								hRem += 1;/* encrease total remove package counter */
								Ack += 1;/* encrease ack's counter */
								printf("Total removed: %s%i%s\n", GREEN, hRem, WHITE);
								printf("Total ack: %s%i%s\n", GREEN, Ack, WHITE);
								break;
							}
						tempRem=tempRem->next;
						if (tempRem==NULL)break;/* exit from while when at the end of the list */
						}/* WHILE TEMP REM */
				}/* IF */

			/* Part of the 'EXIT ROUTINE' protocol. Once 'F'(final package arrived to ProxyReceiver) package arrived send back 'S'(stop connection) package to Proxyreceiver */
			if( ((ACK *)bufACK)->tipo == 'B' && /* if has 'tipo' as 'B' and 'ack' as 'F'... */
				((ACK *)bufACK)->ack == 'F')
				{	
					/* create 'S' package */
					ACK* ack;
					printf("F-Ack for ID: %s%i%s\n", BLUE,ntohl(((ACK *)bufACK)->id_received),WHITE);
					ack = (ACK*)malloc(sizeof(ACK));
					if (ack == NULL){perror("error: ");exit(1);}/* handle malloc error */
					ack->tipo = 'B';
					ack->ack = 'S';
					ack->id_network_order=htonl(ntohl(((BODY *)buf)->id_network_order)+1);
					ack->id_received=htonl(ntohl(((BODY *)buf)->id_network_order)+1);
					send_udp_Sender(UDPsockfd, (char*)ack, 10, RitIP, free_port0, free_port1, free_port2);/* send package */
					free(ack);/* free malloc */
					close(UDPsockfd);/* close listening UDP connection */
					Ack += 1;
					temp=start;
					while(temp!=NULL){/* remove everything on the list of packages */
						removeList(temp);
						temp=temp->next;
						if(temp==NULL)break;
					}
					printf("%s\"CLOSE SESSION\" package sent\nUDP connection closed and exit\n\n%s", RED,WHITE);
					exit(1);
				}/* WHILE FIN */
			
			/* if ICMP arrived close the port(of Ritardatore, so don't send anything tothat port) from where you received the package, and re-send the package. */
			if(((ICMP *)bufACK)->tipo == 'I')
				{	/* 1 means closed (so don't send to them) */
					if ( ntohs(fromRit.sin_port) == 61000)free_port0 = 1;
					if ( ntohs(fromRit.sin_port) == 61001)free_port1 = 1;
					if ( ntohs(fromRit.sin_port) == 61002)free_port2 = 1;
					printf("Received ICMP - id: %i\n", ntohl(((ICMP *)bufACK)->packet_lost_id_network_order));
					icmp++;
					temp=start;
					while(temp!=NULL)
					{
						if( ntohl(((BODY *)temp->Pack)->id_network_order) == 
							ntohl(((ICMP *)bufACK)->packet_lost_id_network_order) )
							{
								send_udp_Sender(UDPsockfd, (char*)temp->Pack, BODYSIZE, RitIP, free_port0, free_port1, free_port2);
								totNH += BODYSIZE;
								break;
							}
							temp=temp->next;
							if (temp==NULL)printf("Error..Package not present in list.");break;
							}
							/*printf("%sTotal ack: %i%s\n", GREEN, Ack, WHITE);*/
							printf("Total bytes sent to UDP socket: %s%i%s\n\n\n", BLUE, totNH, WHITE);
				}/* WHILE ICMP */
			
			/* HurryUp's ack arrived. So re-send the missing package for max 15 times */
			if( ((ACK *)bufACK)->tipo == 'B' &&
				((ACK *)bufACK)->ack == 'H')
				{
					printf("H-Ack for ID: %s%i%s\n", BLUE,ntohl(((ACK *)bufACK)->id_received),WHITE);
					temp=start;
					while(temp!=NULL){
						if( (ntohl(((BODY *)temp->Pack)->id_network_order) == 
							ntohl(((ACK *)bufACK)->id_received)) && hold<15 )
							{
								printf("%shurry: %i%s\n", GREEN, hurry, WHITE);
								if( hurry == ntohl(((ACK *)bufACK)->id_received) )hold++;/* if the hurryup just received is the same of the last encrease HurryUp's counter limit */
								if( hurry != ntohl(((ACK *)bufACK)->id_received) )hold=0;/* if it's different set HurryUp's counter limit to 0 */
								hurry = ntohl(((ACK *)bufACK)->id_received);
								send_udp_Sender(UDPsockfd, (char*)temp->Pack, BODYSIZE, RitIP, free_port0, free_port1, free_port2);
								totNH += BODYSIZE;
								break;
								}
								temp=temp->next;
								if (temp==NULL)break;
								}
								printf("%sTotal ack: %i%s\n", GREEN, Ack, WHITE);
								printf("Total bytes sent to UDP socket: %s%i%s\n\n", BLUE, totNH, WHITE);
					}/* WHILE HURRY */
		
			}/* WHILE ISSET */
	
	if (free_port0 == 1 && free_port1 == 1 && free_port2 == 1){/* correct the worst case...if all ports(of Ritardatore) are closed, set'em free */
			free_port0 = 0;
			free_port1 = 0;
			free_port2 = 0;
		}
	
	if(start){/* print the current package list */
	currentRem=start;
	h=0;
	while(currentRem != NULL)
	{
		printf("%i->", ntohl(((BODY *)currentRem->Pack)->id_network_order));
		currentRem = currentRem->next;
		h++;
		if (currentRem==NULL)break;
	}
	printf("\nLeft over list:%s%i%s\n", GREEN, h, WHITE);
	printf("Total packs \"RECEIVED\" from Sender: %s%i%s\n", GREEN,packTCP,WHITE);
	printf("Total packs RECEIVED from Rit: %s%i%s\n", GREEN,totPacks,WHITE);
	printf("Total ack: %s%i%s\n", GREEN, Ack, WHITE);
	printf("Total ICMP: %s%i%s\n\n", GREEN, icmp, WHITE);
	}
	
	
	}/* WHILE (1) *//* you'll never pass this edge */

    return 0;

}/* MAIN */ 
