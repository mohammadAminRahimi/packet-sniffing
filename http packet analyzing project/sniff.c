/*
	Packet sniffer using libpcap library
*/
#include<pcap.h>
#include<stdio.h>
#include<stdlib.h> // for exit()
#include<string.h> //for memset
#include<syslog.h>



#include<sys/socket.h>
#include<arpa/inet.h> // for inet_ntoa()
#include<net/ethernet.h>
#include<netinet/ip_icmp.h>	//Provides declarations for icmp header
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/tcp.h>	//Provides declarations for tcp header
#include<netinet/ip.h>	//Provides declarations for ip header

void process_packet(u_char *, const struct pcap_pkthdr *, const u_char *);
void process_ip_packet(const u_char * , int);
void print_ip_packet(const u_char * , int);
void print_tcp_packet(const u_char *  , int );
void print_udp_packet(const u_char * , int);
void print_icmp_packet(const u_char * , int );
void PrintData (const u_char * , int);
void log_http_request(const u_char *, int);
void log_http_response(const u_char *, int);
FILE *logfile;
struct sockaddr_in source,dest;
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j;	

int main()
{
	pcap_if_t *alldevsp , *device;
	pcap_t *handle; //Handle of the device that shall be sniffed

	char errbuf[100] , *devname , devs[100][100];
	int count = 1 , n;
	
	//First get the list of available devices
	printf("Finding available devices ... ");
	if( pcap_findalldevs( &alldevsp , errbuf) )
	{
		printf("Error finding devices : %s" , errbuf);
		exit(1);
	}
	printf("Done");
	
	//Print the available devices
	printf("\nAvailable Devices are :\n");
	for(device = alldevsp ; device != NULL ; device = device->next)
	{
		printf("%d. %s - %s\n" , count , device->name , device->description);
		if(device->name != NULL)
		{
			strcpy(devs[count] , device->name);
		}
		count++;
	}
	
	//Ask user which device to sniff
	printf("Enter the number of the device you want to sniff : ");
	scanf("%d" , &n);
	devname = devs[n];
	
	//Open the device for sniffing
	printf("Opening device %s for sniffing ... " , devname);
	handle = pcap_open_live(devname , 65536 , 1 , 0 , errbuf);
	printf("hello world");
	if (handle == NULL) 
	{
		printf("Couldn't open device %s : %s\n" , devname , errbuf);
		exit(1);
	}
	printf("Done\n");
	
	logfile=fopen("log1.txt","w");
	openlog("http protocol message log", LOG_CONS, LOG_USER);
	if(logfile==NULL) 
	{
		printf("Unable to create file.");
	}
	
	//Put the device in sniff loop
	pcap_loop(handle , -1 , process_packet , NULL);
	

	return 0;	
}

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer)
{
	int size = header->len;
	
	//Get the IP Header part of this packet , excluding the ethernet header
	struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
	++total;
	switch (iph->protocol) //Check the Protocol and do accordingly...
	{
		case 1:  //ICMP Protocol
			++icmp;
			// print_icmp_packet( buffer , size);
			break;
		
		case 2:  //IGMP Protocol
			++igmp;
			break;
		
		case 6:  //TCP Protocol
			++tcp;
			print_tcp_packet(buffer , size);
			break;
		
		case 17: //UDP Protocol
			++udp;
			// print_udp_packet(buffer , size);
			break;
		
		default: //Some Other Protocol like ARP etc.
			++others;
			break;
	}
	//printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\r", tcp , udp , icmp , igmp , others , total);
}

void print_ethernet_header(const u_char *Buffer, int Size)
{
	struct ethhdr *eth = (struct ethhdr *)Buffer;
	
	fprintf(logfile , "\n");
	fprintf(logfile , "Ethernet Header\n");
	fprintf(logfile , "   |-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
	fprintf(logfile , "   |-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
	fprintf(logfile , "   |-Protocol            : %u \n",(unsigned short)eth->h_proto);
}

void print_ip_header(const u_char * Buffer, int Size)
{
	print_ethernet_header(Buffer , Size);
  
	unsigned short iphdrlen;
		
	struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr) );
	iphdrlen =iph->ihl*4;
	
	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = iph->saddr;
	
	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = iph->daddr;
	
	fprintf(logfile , "\n");
	fprintf(logfile , "IP Header\n");
	fprintf(logfile , "   |-IP Version        : %d\n",(unsigned int)iph->version);
	fprintf(logfile , "   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
	fprintf(logfile , "   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
	fprintf(logfile , "   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
	fprintf(logfile , "   |-Identification    : %d\n",ntohs(iph->id));
	//fprintf(logfile , "   |-Reserved ZERO Field   : %d\n",(unsigned int)iphdr->ip_reserved_zero);
	//fprintf(logfile , "   |-Dont Fragment Field   : %d\n",(unsigned int)iphdr->ip_dont_fragment);
	//fprintf(logfile , "   |-More Fragment Field   : %d\n",(unsigned int)iphdr->ip_more_fragment);
	fprintf(logfile , "   |-TTL      : %d\n",(unsigned int)iph->ttl);
	fprintf(logfile , "   |-Protocol : %d\n",(unsigned int)iph->protocol);
	fprintf(logfile , "   |-Checksum : %d\n",ntohs(iph->check));
	fprintf(logfile , "   |-Source IP        : %s\n" , inet_ntoa(source.sin_addr) );
	fprintf(logfile , "   |-Destination IP   : %s\n" , inet_ntoa(dest.sin_addr) );
}

void print_tcp_packet(const u_char * Buffer, int Size)
{
	unsigned short iphdrlen;
	
	struct iphdr *iph = (struct iphdr *)( Buffer  + sizeof(struct ethhdr) );
	iphdrlen = iph->ihl*4;
	
	struct tcphdr *tcph=(struct tcphdr*)(Buffer + iphdrlen + sizeof(struct ethhdr));
			
	int header_size =  sizeof(struct ethhdr) + iphdrlen + tcph->doff*4;
	
	fprintf(logfile , "\n\n***********************TCP Packet*************************\n");	
		
	// print_ip_header(Buffer,Size);
		
	// fprintf(logfile , "\n");
	// fprintf(logfile , "TCP Header\n");
	// fprintf(logfile , "   |-Source Port      : %u\n",ntohs(tcph->source));
	// fprintf(logfile , "   |-Destination Port : %u\n",ntohs(tcph->dest));
	// fprintf(logfile , "   |-Sequence Number    : %u\n",ntohl(tcph->seq));
	// fprintf(logfile , "   |-Acknowledge Number : %u\n",ntohl(tcph->ack_seq));
	// fprintf(logfile , "   |-Header Length      : %d DWORDS or %d BYTES\n" ,(unsigned int)tcph->doff,(unsigned int)tcph->doff*4);
	// //fprintf(logfile , "   |-CWR Flag : %d\n",(unsigned int)tcph->cwr);
	// //fprintf(logfile , "   |-ECN Flag : %d\n",(unsigned int)tcph->ece);
	// fprintf(logfile , "   |-Urgent Flag          : %d\n",(unsigned int)tcph->urg);
	// fprintf(logfile , "   |-Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
	// fprintf(logfile , "   |-Push Flag            : %d\n",(unsigned int)tcph->psh);
	// fprintf(logfile , "   |-Reset Flag           : %d\n",(unsigned int)tcph->rst);
	// fprintf(logfile , "   |-Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
	// fprintf(logfile , "   |-Finish Flag          : %d\n",(unsigned int)tcph->fin);
	// fprintf(logfile , "   |-Window         : %d\n",ntohs(tcph->window));
	// fprintf(logfile , "   |-Checksum       : %d\n",ntohs(tcph->check));
	// fprintf(logfile , "   |-Urgent Pointer : %d\n",tcph->urg_ptr);
	// fprintf(logfile , "\n");
	// fprintf(logfile , "                        DATA Dump                         ");
	// fprintf(logfile , "\n");
		
	// fprintf(logfile , "IP Header\n");
	// PrintData(Buffer,iphdrlen);
		
	// fprintf(logfile , "TCP Header\n");
	// PrintData(Buffer+iphdrlen,tcph->doff*4);
		
	// fprintf(logfile , "Data Payload\n");	
	//PrintData(Buffer + header_size , Size - header_size );
	// printf("first\n");
	log_http_request(Buffer + header_size , Size - header_size );
	// printf("second\n");
	log_http_response(Buffer + header_size , Size - header_size );
	// printf("third\n");

	//fprintf(logfile , "\n###########################################################");
}

void print_udp_packet(const u_char *Buffer , int Size)
{
	
	unsigned short iphdrlen;
	
	struct iphdr *iph = (struct iphdr *)(Buffer +  sizeof(struct ethhdr));
	iphdrlen = iph->ihl*4;
	
	struct udphdr *udph = (struct udphdr*)(Buffer + iphdrlen  + sizeof(struct ethhdr));
	
	int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof udph;
	
	fprintf(logfile , "\n\n***********************UDP Packet*************************\n");
	
	print_ip_header(Buffer,Size);			
	
	fprintf(logfile , "\nUDP Header\n");
	fprintf(logfile , "   |-Source Port      : %d\n" , ntohs(udph->source));
	fprintf(logfile , "   |-Destination Port : %d\n" , ntohs(udph->dest));
	fprintf(logfile , "   |-UDP Length       : %d\n" , ntohs(udph->len));
	fprintf(logfile , "   |-UDP Checksum     : %d\n" , ntohs(udph->check));
	
	fprintf(logfile , "\n");
	fprintf(logfile , "IP Header\n");
	PrintData(Buffer , iphdrlen);
		
	fprintf(logfile , "UDP Header\n");
	PrintData(Buffer+iphdrlen , sizeof udph);
		
	fprintf(logfile , "Data Payload\n");	
	
	//Move the pointer ahead and reduce the size of string
	PrintData(Buffer + header_size , Size - header_size);
	
	fprintf(logfile , "\n###########################################################");
}

void print_icmp_packet(const u_char * Buffer , int Size)
{
	unsigned short iphdrlen;
	
	struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr));
	iphdrlen = iph->ihl * 4;
	
	struct icmphdr *icmph = (struct icmphdr *)(Buffer + iphdrlen  + sizeof(struct ethhdr));
	
	int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof icmph;
	
	fprintf(logfile , "\n\n***********************ICMP Packet*************************\n");	
	
	print_ip_header(Buffer , Size);
			
	fprintf(logfile , "\n");
		
	fprintf(logfile , "ICMP Header\n");
	fprintf(logfile , "   |-Type : %d",(unsigned int)(icmph->type));
			
	if((unsigned int)(icmph->type) == 11)
	{
		fprintf(logfile , "  (TTL Expired)\n");
	}
	else if((unsigned int)(icmph->type) == ICMP_ECHOREPLY)
	{
		fprintf(logfile , "  (ICMP Echo Reply)\n");
	}
	
	fprintf(logfile , "   |-Code : %d\n",(unsigned int)(icmph->code));
	fprintf(logfile , "   |-Checksum : %d\n",ntohs(icmph->checksum));
	//fprintf(logfile , "   |-ID       : %d\n",ntohs(icmph->id));
	//fprintf(logfile , "   |-Sequence : %d\n",ntohs(icmph->sequence));
	fprintf(logfile , "\n");

	fprintf(logfile , "IP Header\n");
	PrintData(Buffer,iphdrlen);
		
	fprintf(logfile , "UDP Header\n");
	PrintData(Buffer + iphdrlen , sizeof icmph);
		
	fprintf(logfile , "Data Payload\n");	
	
	//Move the pointer ahead and reduce the size of string
	PrintData(Buffer + header_size , (Size - header_size) );
	
	fprintf(logfile , "\n###########################################################");
}

void PrintData (const u_char * data , int Size)
{
	// printf("hey");
	if(Size < 20){
		return;
	}
	int i , j=0, flag=1;
	// printf("%c %c %c\n\n\n\n", data[0],data[1],data[2]);
	if(data[0]=='G' && data[1]=='E' && data[2]=='T')
		log_http_request(data, Size);
	// for(i=0 ; i<Size ; i++){
	// 	if(flag==1 && j==0){
	// 		if(data[i]=='H' && data[i+1]=='T'  && data[i+2]=='T'  && data[i+3]=='P')
	// 			log_http_response(data, Size);
	// 		flag=0;
	// 	}
	// 	if(flag=1 && j==2){
	// 		printf("\n\n\n\n\n\n\n\n\n%c %c %c %c\n", (unsigned char)data[i], (unsigned char)data[i+1], (unsigned char)data[i+2], (unsigned char)data[i+3]);
	// 		printf("%c %c %c %c \n", data[i], data[i+1], data[i+2], data[i+3]);
	// 		if((unsigned char)data[i]=='H' && (unsigned char)data[i+1]=='T'  && (unsigned char)data[i+2]=='T'  && (unsigned char)data[i+3]=='P'){
	// 			printf("http\n");
	// 			printf("%c %c %c %c", (unsigned char)data[i], (unsigned char)data[i+1], (unsigned char)data[i+2], (unsigned char)data[i+3]);
	// 			log_http_request(data, Size);
	// 		}
	// 		flag=0;
	// 	}	
	// 	if(j==2)
	// 		break;
	// 	if(data[i]==' ')
	// 		flag=1;
	// 		j++;
	// }


	// original function
	// for(i=0 ; i < Size ; i++)
	// {
	// 	if(i!=0 && i%16==0)   //if one line of hex printing is complete...
	// 	{
	// 		fprintf(logfile , "         ");
	// 		for(j=i-16 ; j<i ; j++)
	// 		{
	// 			if(data[j]>=32 && data[j]<=128)
	// 				fprintf(logfile , "%c",(unsigned char)data[j]); //if its a number or alphabet
				
	// 			else fprintf(logfile , "."); //otherwise print a dot
	// 		}
	// 		fprintf(logfile , "\n");
	// 	} 


		// HEX PRINTING
		// if(i%16==0) fprintf(logfile , "   ");
		// 	fprintf(logfile , " %02X",(unsigned int)data[i]);



		//	 if( i==Size-1)  //print the last spaces
		// {
		// 	for(j=0;j<15-i%16;j++) 
		// 	{
		// 	  fprintf(logfile , "   "); //extra spaces
		// 	}
			
		// 	fprintf(logfile , "         ");
			
		// 	for(j=i-i%16 ; j<=i ; j++)
		// 	{
		// 		if(data[j]>=32 && data[j]<=128) 
		// 		{
		// 		  fprintf(logfile , "%c",(unsigned char)data[j]);
		// 		}
		// 		else 
		// 		{
		// 		  fprintf(logfile , ".");
		// 		}
		// 	}
			
		// 	fprintf(logfile ,  "\n" );
		// }
	// }
}

void log_http_request(const u_char *data, int size){
	char dataCopy[size +10];
	strcpy(dataCopy, data);
	if(size<20)
		return;
	char *token = strtok(dataCopy, " ");
	if(token==NULL)
		return;
	if(strcmp(token, "GET")==0 || strcmp(token, "PUT")==0 || strcmp(token, "POST")==0 || strcmp(token, "DELETE")==0){
		char string[size+100];
		int counter =  strlen("\n\n\n\n\n\n       http request logging\n\n");
		strcpy(string,"\n\n\n\n\n\n       http request logging\n\n" );
		strcat(string, token);
		strcat(string, " ");
		strcat(string, strtok(NULL, " "));
		strcat(string, " ");
		token = strtok(NULL, "\0");
		strcat(string, token);
		if(token==NULL || strlen(token)<5)
			return;
		if(*token=='H' && *(token+1)=='T' && *(token+2)=='T' && *(token+3)=='P'){
			printf("request begin\n");
			printf(" %s \n", string);
			printf("request end\n\n\n");

			// for(int i=0 ; i<size-1 ; i++ ){
			// 	// printf("%c", data[i]);
			// 	string[counter]= data[i];
			// 	counter++;
			// 	if(i < size-4)
			// 	if(data[i]==13 && data[i+1]==10 && data[i+2]==13 && data[i+3]==10)
			// 		break;
			// }
			// printf("string1 \n %s \n string", string);
		}
	}
	// printf("fun end");
}

void log_http_response(const u_char * data, int size){
	if(size<20)
		return;
	char *token = strtok(data, " ");
	if(token==NULL || strlen(token)<5)
		return;
	if(*token=='H' && *(token+1)=='T' && *(token+2)=='T' && *(token+3)=='P'){
		char string[size+100];
		strcpy(string,"\n\n\n\n\n\n       http response logging\n\n" );
		strcat(string, token);
		strcat(string, " ");
		int counter = strlen(string);
		data = strtok(NULL, "\0");
		strcat(string, token);
		// printf("here %s here", token);
		// printf("%s", string);
		for(int i=0 ; i<strlen(data) ; i++ ){
			// printf("%c", data[i]);
			string[counter++]=data[i];
			if(i < size-4)
			if(data[i]==13 && data[i+1]==10 && data[i+2]==13 && data[i+3]==10)
				break;
		}
		string[counter]='\0';
		syslog(LOG_INFO, "%s", string);
		printf("%s", string);
	}
}