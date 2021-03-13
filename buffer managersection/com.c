#include "distcommon.h"

int addMessageToNode(tMsg msg)
{	
	tMsg *new_msg=(tMsg *)malloc(sizeof(tMsg));
	new_msg->nodeNumber=msg.nodeNumber;
	new_msg->timeStamp=msg.timeStamp;
	printf("New Node %d  time %d",new_msg->nodeNumber, new_msg->timeStamp);	
}

int acceptClient(connectObj cObj)
{
	int fromlen, new_sock, read_return;
	fromlen = sizeof(struct sockaddr);	
		
//	printf("\nServer is waiting to accept a client");	
	while((new_sock = accept(cObj.inet_sock, (struct sockaddr *) cObj.inet_telnum, 
						&fromlen)) == -1 && errno == EINTR);	
	if(new_sock == -1)
	{
		perror("accept failed: ");
		exit(2);
	}
	
	else
	{
//		printf("\nServer accepted the connection -soc : %d",new_sock);		
	}	
	return new_sock;	
}


tMsg receiveMessage(int new_sock)
{
	tMsg recMsg;
	int read_return;
		read_return=read(new_sock,&recMsg,sizeof(recMsg));
	printf("\nGot MessageNode - %d timeStamp %d",recMsg.nodeNumber,recMsg.timeStamp);
	return recMsg;
}

tMsg createMessage(int nodeNumber, int timestamp)
{
	tMsg msg;
	msg.nodeNumber=nodeNumber;
	msg.timeStamp=timestamp;
	return msg;
}
void printMessage(tMsg msg)
{
	printf("Node :%d TimeStamp :%d\n",msg.nodeNumber,msg.timeStamp);
}

tMsg createMessage_console()
{
	tMsg msg;
	
	printf("Enter Node Name & TimeStamp : \n");
	scanf("%d %d",&msg.nodeNumber,&msg.timeStamp);	
	printMessage(msg);
	return msg;
}



tMsg sendMessage(int new_sock,tMsg msg)
{			
	//printMessage(msg);	
	
	if(write(new_sock, &msg,sizeof(msg)) == -1)
	{
          perror("inet_sock write failed: ");
          exit(3);
	}
	else
		return msg;
}
connectObj connectToClient(char hostname[20],int portNumber)
{
		int     inet_sock;	
		union type_size;
		struct sockaddr_in inet_telnum;
		struct hostent *heptr, *gethostbyname();
				
		printf("Connect to Client with port  %d\n",portNumber);
		/***** allocate a socket to communicate with *****/

		        if((inet_sock=socket(AF_INET, SOCK_STREAM, 0)) == -1){
		          perror("inet_sock allocation failed: ");
		          exit(1);
		        }

		/***** get a hostent pointer to point to a hostent structure *****/
		/***** which contains the remote IP address of server        *****/

			//if((heptr = gethostbyname( argv[1] )) == NULL){
			if((heptr = gethostbyname(hostname)) == NULL){
				perror("gethostbyname failed: ");
			  exit(1);
			}
		 
		/***** byte copy the IP adress from the h_addr field in the *****/
		/***** hostent structure into an IP address structure       *****/

			bcopy(heptr->h_addr, &inet_telnum.sin_addr, heptr->h_length);
			inet_telnum.sin_family = AF_INET;
			inet_telnum.sin_port = htons( (u_short)portNumber);

		/***** use the connect system call to open a TCP connection *****/

			if(connect(inet_sock, (struct sockaddr *)&inet_telnum, 
		                               sizeof(struct sockaddr_in)) == -1)
			{
		          perror("inet_sock connect failed: ");
		          exit(2);
		    }
			
			
			else
			{
				connectObj con;
				con.inet_sock=inet_sock;
				return con;
			}	
}

connectObj serverListen(int portNumber)
{
		int  inet_sock, new_sock;
		char *myBuffer;
		struct sockaddr_in 	inet_telnum;
		struct hostent 		*heptr, *gethostbyname();
		int	wild_card = INADDR_ANY;
		
		int fromlen;
		int read_return;
		tMsg recMsg;
		
		
		if((inet_sock=socket(AF_INET, SOCK_STREAM, 0)) == -1){
				perror("inet_sock allocation failed: ");
				exit(1);
			}
		
		bcopy(&wild_card, &inet_telnum.sin_addr, sizeof(int));
		inet_telnum.sin_family = AF_INET;
		inet_telnum.sin_port = htons( (u_short)portNumber);
		if(bind(inet_sock, (struct sockaddr *)&inet_telnum, 
				sizeof(struct sockaddr_in)) == -1)
		{
			perror("inet_sock bind failed: ");
			exit(2);
		}
		listen(inet_sock, 5);		
		
		fromlen = sizeof(struct sockaddr);
		printf("\nServer is listening on port %d\n",portNumber);
		connectObj r_connectObj;
		r_connectObj.inet_sock=inet_sock;
		r_connectObj.inet_telnum=(struct sockaddr *)&inet_telnum;
		r_connectObj.fromlen=&fromlen;		
			
		return r_connectObj;	
		
}
