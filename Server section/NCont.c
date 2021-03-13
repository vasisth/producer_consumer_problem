#include "distcommon.h"
#include "pthread.h"

pthread_t	thread_id [30], sig_wait_id;

pthread_attr_t 		thread_attr;

pthread_mutex_t	allNodes_status;
pthread_mutex_t gotMessage;
pthread_mutex_t m_newMsg_nc;
pthread_mutex_t m_noMsg_srv_thds;
pthread_mutex_t m_criticalSection_threadId;
pthread_mutex_t m_nc_producer;
pthread_mutex_t m_print;

pthread_cond_t cond_nc_producer;
pthread_cond_t cond_criticalSection_threadId;
pthread_cond_t	cond_allNodes_status;
pthread_cond_t  cond_gotMessage;
pthread_cond_t  cond_m_newMsg_nc;
pthread_cond_t  cond_m_noMsg_srv_thds;
pthread_cond_t cond_print;

int nodesReady=0;
int newMessage=0; // 1 implies a node has written 2. implies the message is read by NC.

tMsg nc_newMsg;
int newMsg_raiseflag;
int numberOfNodes;
int systemNodeNumber;
int timestamp;

int serialCounter[NUMFLAVORS];

int criticalSection_threadId;
int criticalSection_timeStamp;
int curr_soc[3];
donut_request producedDonut;
//

int systemNodeNumber;
int numberOfNodes;
struct_cs_reqNode *startNode;
struct_cs_reqNode *selfNode_currRequest;
struct_cs_reqNode *lastNode;


//

tMsg createMessage_console();
tMsg createMessage(int nodeNumber, int timestamp);
tMsg sendMessage(int curr_soc,tMsg msg);
int addMessageToNode(tMsg msg);
tMsg receiveMessage(int curr_soc);
connectObj connectToClient(char hostname[20],int portNumber);
connectObj serverListen(int portNumber);
int acceptClient(connectObj cObj);
void printMessage(tMsg msg);

void printNode(struct_cs_reqNode *tempNode)
{
	int i=0;
	printf("\n\t ***StartPrint Node");
	while(tempNode->msg[i]!=NULL)
	{
		printf("\n\tMessage: %d :NodeNo %d :TimeStamp %d",i,tempNode->msg[i]->nodeNumber,tempNode->msg[i]->timeStamp);
		i++;
	}
	printf("\n\t ***ENDPrint Node");
}
void printAllNodes()
{
	struct_cs_reqNode *tempNode=startNode;
	int i=0;
	int j=1;
	printf("\n----------------print nodes------------------");
	while(tempNode!=NULL)
	{		
		printf("\nNode %d",j);
		i=0;
		j++;
		while(tempNode->msg[i]!=NULL&&i<numberOfNodes)
		{
			printf("\nMessage: %d :NodeNo %d :TimeStamp %d",i,tempNode->msg[i]->nodeNumber,tempNode->msg[i]->timeStamp);
			i++;
		}
		tempNode=tempNode->nextNode;
	}
	printf("\n################END Printing of Nodes!!!!##############");	
}

struct_cs_reqNode * insertafterNode(tMsg msg)
{
	struct_cs_reqNode*tempNode=startNode;
	struct_cs_reqNode *temp2=NULL;
	struct_cs_reqNode *nextNode=startNode->nextNode;
	
	while((nextNode !=NULL)&&(nextNode->msg[0]->timeStamp < msg.timeStamp))
	{
		
		tempNode=nextNode;
		nextNode=tempNode->nextNode;
	}
	if((nextNode!=NULL)&&(tempNode->msg[0]->timeStamp == msg.timeStamp)&& ( nextNode->msg[0]->timeStamp== msg.timeStamp))
	{		
		temp2=tempNode;
		while((tempNode !=NULL)&&(tempNode->msg[0]->nodeNumber < msg.nodeNumber) && (tempNode->msg[0] -> timeStamp == msg.timeStamp))
		{
			temp2=tempNode;
			tempNode=tempNode->nextNode;
			//nextNode=tempNode->nextNode;
		}		
		tempNode=temp2;
	}
	else if((nextNode!=NULL)&&(tempNode->msg[0]->timeStamp < msg.timeStamp)&& ( nextNode->msg[0]->timeStamp== msg.timeStamp))
	{		
		while((nextNode !=NULL)&&(nextNode->msg[0]->nodeNumber < msg.nodeNumber) && (nextNode->msg[0] -> timeStamp == msg.timeStamp))
		{			
			tempNode=nextNode;
			nextNode=tempNode->nextNode;
		}		
	}
	
	return tempNode;	
}
void sendCSRequestToAllNodes(tMsg msg)
{
	int i=1;
	printf("\nSend Message  %d , %d to All Nodes",msg.nodeNumber, msg.timeStamp);
	while(i<4)
	{		
		if(i!=msg.nodeNumber)
		{		
			printf("\n Send i Value %d",i);
			sendMessage(curr_soc[i],msg);
		}
		i++;
	}
}

void replyToOtherNode(tMsg msg,int *nodeTimestamp)
{
	struct_cs_reqNode *tempNode, *tmp;
	tMsg *tempMsg;
	tempNode=startNode;
	
	while(!((tempNode->msg[0]->nodeNumber==msg.nodeNumber) 
		&& (tempNode->msg[0]->timeStamp==msg.timeStamp)))		
	{
		
		tempNode=tempNode->nextNode;
	}
	printf("\n\t REPLY to :Node %d  :reqTimeStamp %d :ReplytimeStamp %d",msg.nodeNumber,msg.timeStamp,*nodeTimestamp);
	tempMsg=(tMsg *)malloc(sizeof(tMsg));
	tempMsg->nodeNumber=systemNodeNumber;
	tempMsg->timeStamp=*(nodeTimestamp);
	sendMessage(curr_soc[msg.nodeNumber],*tempMsg);
	tempNode->msg[1]=tempMsg;	
	printNode(tempNode);
	*(nodeTimestamp)=*(nodeTimestamp)+1;
}

void addNewNode(tMsg msg,int *nodeTimestamp)
{
	struct_cs_reqNode *node;
	struct_cs_reqNode *insert_point;
	struct_cs_reqNode *tempNode;
	tMsg *tempMsg;
	printf("\n Add New Node:%d : TS:%d  :NodeTimeStamp:%d",msg.nodeNumber,msg.timeStamp,*nodeTimestamp);
	fflush(stdout);
	node=(struct_cs_reqNode *)malloc(sizeof(struct_cs_reqNode));	
	tempMsg=(tMsg*)malloc(sizeof(tMsg));	
	node->msg[0]=tempMsg;
	node->msg[0]->nodeNumber=msg.nodeNumber;
	node->msg[0]->timeStamp=msg.timeStamp;	
	node->msg[0]->threadId=msg.threadId;
	node->nextNode=NULL;
	node->msg[1]=NULL;
	node->msg[2]=NULL;
	node->totalResponses=1;
	
	
	if(startNode==NULL)
	{		
		startNode=node;
	}
	else
	{		
		if(startNode->nextNode==NULL)
		{
			
			if(startNode->msg[0]->timeStamp < msg.timeStamp)
			{				
				//cannot be from same node;
				startNode->nextNode=node;
				
				
			}
			else if(startNode->msg[0]->timeStamp > msg.timeStamp)
			{
				node->nextNode=startNode;					
			 	startNode=node;
			}
			else if(startNode->msg[0]->timeStamp == msg.timeStamp && startNode->msg[0]->nodeNumber <  msg.nodeNumber)
			{
				
				startNode->nextNode=node;
			}
			else if(startNode->msg[0]->timeStamp == msg.timeStamp && startNode->msg[0]->nodeNumber >  msg.nodeNumber)
			{
				
				node->nextNode=startNode;
				startNode=node;				
			}
			else
			{
				
			}
		}
		else
		{
			insert_point=insertafterNode(msg);
			
			if(startNode==insert_point)
			{
				if(startNode->msg[0]->timeStamp > msg.timeStamp)
				{
					node->nextNode=startNode;
					startNode=node;					
				}
				else if(startNode->msg[0]->timeStamp < msg.timeStamp)
				{					
					node->nextNode=startNode->nextNode;
					startNode->nextNode=node;				
				}
				else if(startNode->msg[0]->timeStamp == msg.timeStamp && startNode->msg[0]->nodeNumber > msg.nodeNumber)
				{
					
					node->nextNode=startNode;
					startNode=node;			
				}
				else if(startNode->msg[0]->timeStamp == msg.timeStamp && startNode->msg[0]->nodeNumber < msg.nodeNumber)
				{
					node->nextNode=startNode->nextNode;
					startNode->nextNode=node;
				}
				else
				{	
				}
			}
			else
			{				
				node->nextNode=insert_point->nextNode;
				insert_point->nextNode=node;
				if(msg.nodeNumber==selfNode_currRequest)
				{
					*nodeTimestamp=(*nodeTimestamp)+1;
				}
			}
		}
			
			
	}
	if(selfNode_currRequest==NULL)
	{
		if(msg.nodeNumber==systemNodeNumber)
		{
			
			selfNode_currRequest=node;
			msg.timeStamp=*nodeTimestamp;
			
			sleep(5);
			sendCSRequestToAllNodes(msg);
			*nodeTimestamp=*nodeTimestamp+1;
		}
		else
		{			
			//reply to the node in the message.
			replyToOtherNode(msg,nodeTimestamp);
			
		}
	}
	else if(msg.nodeNumber==systemNodeNumber)
	{
		*nodeTimestamp=*nodeTimestamp+1;
	}	
	printAllNodes();	
}


//delete invalid nodes
void deleteInvalidNodes()
{
	struct_cs_reqNode *tempNode;	
	tempNode=startNode;	
	while(tempNode->msg[0]->nodeNumber!=systemNodeNumber)
	{
		//delete the node
		free(tempNode->msg[0]);
		free(tempNode->msg[1]);
		free(tempNode);
		tempNode=tempNode->nextNode;
	}
	startNode=tempNode->nextNode;
	if(startNode==NULL)
	{
		printf("\n\t <<<START NODE is NULL>>>>\n\n");
	}
	free(tempNode->msg[0]);
	free(tempNode->msg[1]);
	free(tempNode->msg[2]);
	free(tempNode);
	// Assigning new StartNode
	tempNode=startNode;
	while((tempNode!=NULL)&&(tempNode->msg[0]->nodeNumber!=systemNodeNumber))
	{
		tempNode=tempNode->nextNode;
	}
	selfNode_currRequest=tempNode;	
}

void    *thd_serverListen ( void *arg )
{	
	int port_number,ti;	
	int i;
	connectObj c_obj;
	tMsg msg;
	int status_AllNodesReady=0;
	
	switch(*(int *)arg)
	{ 
	  case 1: ti=1; break;	
	  case 2: ti=2; break;
	  case 3: ti=3; break;	  
	  default : break; 
	}
	port_number=portNumber[ti];
	if(systemNodeNumber==1)
	{
		//listen on 2		
		//listen on 1		
		printf("\nServerListens on port %d Machine %s",port_number,hostNames[ti]);
		
		c_obj=serverListen(port_number);
		printf("\nConn Socket %d - number %d",ti,c_obj.inet_sock);
		curr_soc[ti]=acceptClient(c_obj);
		msg=receiveMessage(curr_soc[ti]);
				
		if(msg.timeStamp==0&&status_AllNodesReady==0)
		{
			pthread_mutex_lock(&allNodes_status);    
			nodesReady++;
			
			printf("\n<<Number of Nodes Ready :%d >>\n",nodesReady);			
			pthread_mutex_unlock(&allNodes_status);
			if(nodesReady==numberOfNodes-1)
			{
				pthread_cond_signal(&cond_allNodes_status);
				status_AllNodesReady=1;
			}
		}	
	}
	if(systemNodeNumber==2)
	{
		//connect to 1 
		if(*(int *)arg==1)
		{
			printf("\nServer connects to port  %d  Machine %s ",portNumber[2],hostNames[*(int *)arg]);
			c_obj=connectToClient(hostNames[*(int *)arg],portNumber[2]);
			printf("\nConn Socket %d - number %d",ti,c_obj.inet_sock);
			curr_soc[ti]=c_obj.inet_sock;
			msg=createMessage(systemNodeNumber,0);
			sendMessage(curr_soc[ti],msg);
			pthread_mutex_lock(&allNodes_status);    
			nodesReady++;			
			printf("\n<<Number of Nodes Ready :%d >>\n",nodesReady);			
			pthread_mutex_unlock(&allNodes_status);
			if(nodesReady==numberOfNodes-1)
			{
				pthread_cond_signal(&cond_allNodes_status);
				status_AllNodesReady=1;
			}
		}
		//listen on 3;
		if(*(int *)arg==3)
		{	
			printf("\nServerListens on port %d Machine %s ",portNumber[1],hostNames[1]);
			c_obj=serverListen(portNumber[1]);
			printf("\nConn Socket %d - number %d",ti,c_obj.inet_sock);
			curr_soc[ti]=acceptClient(c_obj);
			msg=receiveMessage(curr_soc[ti]);
					
			if(msg.timeStamp==0&&status_AllNodesReady==0)
			{
				pthread_mutex_lock(&allNodes_status);    
				nodesReady++;
				
				printf("\n<<Number of Nodes Ready :%d >>\n",nodesReady);			
				pthread_mutex_unlock(&allNodes_status);
				if(nodesReady==numberOfNodes-1)
				{
					pthread_cond_signal(&cond_allNodes_status);
					status_AllNodesReady=1;
				}
			}	
		}
			//listen
		
	}
	if(systemNodeNumber==3)
	{
		//connect to 1, 2;	
		if(ti==1)
		{
			printf("\nServer connects to port  %d Machine %s",portNumber[3],hostNames[*(int *)arg]);
			c_obj=connectToClient(hostNames[*(int *)arg],portNumber[3]);
			printf("\nConn Socket %d - number %d",ti,c_obj.inet_sock);
		}
		else
		{
			printf("\nServer connects to port  %d Machine %s",portNumber[1], hostNames[*(int *)arg]);
			c_obj=connectToClient(hostNames[*(int *)arg],portNumber[1]);
			printf("\nConn Socket %d - number %d",ti,c_obj.inet_sock);
		}
		curr_soc[ti]=c_obj.inet_sock;
		msg=createMessage(systemNodeNumber,0);
		sendMessage(curr_soc[ti],msg);
		pthread_mutex_lock(&allNodes_status);    
		nodesReady++;			
		printf("\n<<Number of Nodes Ready :%d >>\n",nodesReady);			
		pthread_mutex_unlock(&allNodes_status);
		if(nodesReady==numberOfNodes-1)
		{
			pthread_cond_signal(&cond_allNodes_status);
			status_AllNodesReady=1;
		}		
	}
	pthread_mutex_lock(&allNodes_status);	
	while(nodesReady<numberOfNodes-1)
	{
		printf("\n Thread Waits for other threads to initialize.\n");
		pthread_cond_wait(&cond_allNodes_status, &allNodes_status);		
	}	
	pthread_mutex_unlock(&allNodes_status);	
	while(1)
	{
	
		printf("\n Calling recieve socket %d -sockNum %d",ti,curr_soc[ti]);	
		
		msg=receiveMessage(curr_soc[ti]);			
			//pthread_mutex_lock(&gotMessage);
		 	pthread_mutex_lock(&m_newMsg_nc);
		 	while(newMsg_raiseflag==1)
		 	{
		 		printf("Thread Wait for NC - to read the new Message\n");
		 		pthread_cond_wait(&cond_m_newMsg_nc,&m_newMsg_nc);		 		
		 	}
		 	nc_newMsg=msg;
		 	newMsg_raiseflag=1;
		 	printMessage(nc_newMsg);
		 	pthread_mutex_unlock(&m_newMsg_nc);
		 	pthread_cond_signal(&cond_m_newMsg_nc);
		 	//pthread_mutex_unlock(&gotMessage);
		
	}
}

void *thd_startNodeController()
{ 	
	//SetUp the connections;
	int i;
	int count;
	int nodeTimestamp=1;
	count=0;
	int tempProducerThdId;
	//
	int found,span;
	tMsg *tempMsg;
	tMsg tempMsg3;	
	int reply;
	struct_cs_reqNode *tmpNode, *tmpNode2;
	
	pthread_mutex_lock(&allNodes_status);	
	while(nodesReady<numberOfNodes-1)
	{
		printf("\n NC- Wait-while all other Nodes are connected\n");
		pthread_cond_wait(&cond_allNodes_status, &allNodes_status);		
	}	
	pthread_mutex_unlock(&allNodes_status);
	pthread_cond_signal(&cond_allNodes_status);
	pthread_cond_signal(&cond_allNodes_status);
	printf ( "\n  ******* Node Controller Ready For Messages ********** \n" );
	fflush(stdout);
	startNode=NULL;
	selfNode_currRequest=NULL;
	
	while(1)
	{		
		count++;
		pthread_mutex_lock(&m_newMsg_nc);
		while(newMsg_raiseflag==0)
		{
			printf("\nNC: Waiting for New Msgs-");
			pthread_cond_wait(&cond_m_newMsg_nc,&m_newMsg_nc);
		}
		if(nc_newMsg.nodeNumber!=systemNodeNumber)
		{
			printf("\nNew Msg:Other Node: %d :TS: %d",nc_newMsg.nodeNumber,nc_newMsg.timeStamp);
			if(nc_newMsg.timeStamp>=nodeTimestamp)
			{
				timestamp=nc_newMsg.timeStamp;
				nodeTimestamp=nc_newMsg.timeStamp+1;
			}
		}
		else			
		{
			
			nc_newMsg.timeStamp=nodeTimestamp;
			printf("\nSELF New Msg:Same: Node: %d, TS: %d",nc_newMsg.nodeNumber,nc_newMsg.timeStamp);
			
			fflush(stdout);
		}
		//***********************************************
		reply==1;
		if(startNode==NULL)
		{	
			//reply will be done by the node
			addNewNode(nc_newMsg,&nodeTimestamp);			
			/*if(nc_newMsg.nodeNumber==systemNodeNumber)
			{
				nodeTimestamp++;
			}*/
		}
		else if(nc_newMsg.nodeNumber==systemNodeNumber)
		{	
			// no need of incrementing the time stamp... it will be done by the addNewNode
			addNewNode(nc_newMsg,&nodeTimestamp);
		}		
		else if(nc_newMsg.nodeNumber!=systemNodeNumber)
		{		
			if(selfNode_currRequest==NULL)
			{
				addNewNode(nc_newMsg,&nodeTimestamp);				
				//has to reply immediately
			}			
			else if(nc_newMsg.timeStamp < selfNode_currRequest->msg[0]->timeStamp)
			{				
				addNewNode(nc_newMsg,&nodeTimestamp);
				replyToOtherNode(nc_newMsg,&nodeTimestamp);
			}
			else if(nc_newMsg.timeStamp == selfNode_currRequest->msg[0]->timeStamp)
			{				
				addNewNode(nc_newMsg,&nodeTimestamp);
				if(nc_newMsg.nodeNumber<systemNodeNumber)
				{					
					replyToOtherNode(nc_newMsg,&nodeTimestamp);
				}
			}
			else if(nc_newMsg.timeStamp>selfNode_currRequest->msg[0]->timeStamp)
			{				
				i=1;
				found=0;
				span=selfNode_currRequest->totalResponses;				
				
				while(i<numberOfNodes && found==0)
				{					
					if(selfNode_currRequest->msg[i]!=NULL)
					{
						if(selfNode_currRequest->msg[i]->nodeNumber==nc_newMsg.nodeNumber)
						{							
							addNewNode(nc_newMsg,&nodeTimestamp);
							found=-1;
						}
						else
						{
							i++;
						}
					}
					else
					{
						found=i;
					}				
				}				
				if(found==(numberOfNodes-1))
				{
					printf("\n NC - Start deleting");
					//creating the reply.. Append the message to the last node of the selfNode_currRequest
					tempMsg=(tMsg*)malloc(sizeof(tMsg));
					tempMsg->nodeNumber=nc_newMsg.nodeNumber;
					tempMsg->timeStamp=nc_newMsg.timeStamp;
					selfNode_currRequest->msg[found]=tempMsg;
					
					tempProducerThdId=selfNode_currRequest->msg[0]->threadId;
					
					printf("\n\t NC Before Deleting- Message Que ");
					
					printAllNodes();
					deleteInvalidNodes();
					//
					//  Allow producer to produce	
					pthread_mutex_lock(&m_criticalSection_threadId);
					criticalSection_threadId=tempProducerThdId;
					pthread_mutex_unlock(&m_criticalSection_threadId);	
					i=numberOfNodes;
					while(i<(numberOfNodes+1+NUMPRODUCERS))
					{						
						pthread_cond_signal(&cond_criticalSection_threadId);
						i++;
					}
					pthread_mutex_lock(&m_nc_producer);
					while(criticalSection_threadId!=0)
					{
						pthread_cond_wait(&cond_nc_producer,&m_nc_producer);
					}
					pthread_mutex_unlock(&m_nc_producer);
					
					printf("/n NC-  Returned from producer");
					//
					
					if(startNode!=NULL)
					{
						
						tmpNode=startNode;
						i=0;
						while((tmpNode!=NULL)&&(tmpNode->msg[0]->nodeNumber!=systemNodeNumber))
						{
								replyToOtherNode(*(tmpNode->msg[0]),&nodeTimestamp);
								tmpNode2=tmpNode->nextNode;
								tmpNode=tmpNode2;
								
						}
						if(selfNode_currRequest!=NULL)
						{
							tempMsg3=*(selfNode_currRequest->msg[0]);
							tempMsg3.timeStamp=nodeTimestamp;
							sendCSRequestToAllNodes(tempMsg3);
							nodeTimestamp++;
						}					
					}
					printf("\n\tNC- After Deleting- Message Que ");
					printAllNodes();
				}
				else if((found>0)&&(found<(numberOfNodes-1)))
				{
					printf("\nAppend to current node");
					tempMsg=(tMsg*)malloc(sizeof(tMsg));
					tempMsg->nodeNumber=nc_newMsg.nodeNumber;
					tempMsg->timeStamp=nc_newMsg.timeStamp;
					selfNode_currRequest->msg[found]=tempMsg;
					selfNode_currRequest->totalResponses=selfNode_currRequest->totalResponses+1;
					//only append the node 
				}			
			}		
			else
			{
				printf("\n BUG BUG \n ");
			}
		} 		
		//***********************************************		
		newMsg_raiseflag=0;
		pthread_mutex_unlock(&m_newMsg_nc);
		for(i=0;i<numberOfNodes+NUMPRODUCERS;i++)
		{
			pthread_cond_signal(&cond_m_newMsg_nc);
		}
		
	}		
}

int produceRemoteDonut(donut_request  bm_req)
{
	connectObj conn1;
	int pc_req_result, req_complete;
	prodOrConsumeRequest pc_req;
	conn1=connectToClient(hostNames[BM_NODE_INDEX],portNumber[BM_NODE_INDEX]);
	pc_req.prodOrConsume=BM_PRODUCE_REQUEST;	
	pc_req.flavor=bm_req.donutType;
	pc_req.nodeNumber=bm_req.nodeNumber;	
	printf("\n\n PRODUCE DONUT REQUEST: Type:%d Node:%d value:%d\n\n",bm_req.donutType,bm_req.nodeNumber,bm_req.donutValue);
	if(write(conn1.inet_sock, &pc_req,sizeof(prodOrConsumeRequest)) == -1)
	{
          perror("inet_sock write failed: ");
          exit(3);
	}
	if(read(conn1.inet_sock, &pc_req_result,sizeof(int)) == -1)
	{
          perror("inet_sock write failed: ");
          exit(3);
	}	
	if(pc_req_result==11)
	{
		printf("\nBM request is failed-NO SPACE");
		return 0;	
	}
	else //if(pc_req_result == BM_REQUEST_SUCCESS)
	{			
		if(write(conn1.inet_sock, &bm_req,sizeof(donut_request)) == -1)
		{
	          perror("inet_sock write failed: ");
	          exit(3);
		}
		if(read(conn1.inet_sock, &req_complete,sizeof(int)) == -1)
		{
	          perror("inet_sock write failed: ");
	          exit(3);
		}		
	}
	return 1;
}

void	*producer ( void *arg )
{
	int	 j,id;
	int count;
	unsigned short 	xsub[3];
	struct timeval 	randtime;
	tMsg msg;
	donut_request myDonut;
	prodOrConsumeRequest pc_req;
	struct_Mutex *tempMutex;

	gettimeofday ( &randtime, ( struct timezone * ) 0 );	
	xsub[0]=(unsigned short)randtime.tv_sec;
	xsub[1]=(unsigned short)randtime.tv_sec>>16;
	xsub[2]=(unsigned short)(pthread_self);
	
	int productionCount=0;
	count=0;
	
	printf("\n\t Producer - Number of donuts produced %d",(NUM_OF_DOZENS*DOZEN));
	while (count<(NUM_OF_DOZENS*DOZEN)) 

	{
		count++;
		productionCount++;
		//get the random number generator
		j = nrand48 ( xsub ) & 3;		
		printf("\nJJ  %d",j);
	 	pthread_mutex_lock(&m_newMsg_nc);
	 	while(newMsg_raiseflag==1)
	 	{
	 		printf("\nProducer-Thread Wait for NC - to read the new Message\n");
	 		pthread_cond_wait(&cond_m_newMsg_nc,&m_newMsg_nc);		 		
	 	}	 	
	 	msg.nodeNumber=systemNodeNumber;	 	
	 	msg.threadId=*(int *)arg;
	 	nc_newMsg=msg;
	 	printf("\n Producer %d  of threadId %d -wants to produce",msg.nodeNumber,msg.threadId);
	 	newMsg_raiseflag=1;	 	
	 	pthread_mutex_unlock(&m_newMsg_nc);
	 	pthread_cond_signal(&cond_m_newMsg_nc);
	 	
		pthread_mutex_lock(&m_criticalSection_threadId);
		while(criticalSection_threadId!=*(int *)arg)
		{
			printf("\nThread %d waiting for its critical section ",*(int *)arg);
			pthread_cond_wait(&cond_criticalSection_threadId,&m_criticalSection_threadId);
		}	
		
		myDonut.donutType=j;
		myDonut.donutValue=serialCounter[j];
		serialCounter[j]++;
		myDonut.prodThdId=*(int *)arg;
		myDonut.nodeNumber=systemNodeNumber;
		myDonut.timeStamp=criticalSection_timeStamp;
		if(produceRemoteDonut(myDonut)==1)
		{
			printf("Thread %d Production Completed",*(int *)arg);
		}
		else
		{
			serialCounter[j]--;
			printf("Thread %d Production Failed",*(int *)arg);
		}
		
		criticalSection_threadId=0;
		
		pthread_cond_signal(&cond_nc_producer);
		
		pthread_mutex_unlock(&m_criticalSection_threadId);
		
	}	
	
}

int consumeRemoteDonut(donut_request  bm_req)
{
	connectObj conn1;
	int pc_req_result, req_complete;
	prodOrConsumeRequest pc_req;
	conn1=connectToClient(hostNames[BM_NODE_INDEX],portNumber[BM_NODE_INDEX]);
	pc_req.prodOrConsume=BM_CONSUME_REQUEST;	
	pc_req.flavor=bm_req.donutType;
	pc_req.nodeNumber=bm_req.nodeNumber;
	
	if(write(conn1.inet_sock, &pc_req,sizeof(prodOrConsumeRequest)) == -1)
	{
          perror("inet_sock write failed: ");
          exit(3);
	}
	if(read(conn1.inet_sock, &pc_req_result,sizeof(int)) == -1)
	{
          perror("inet_sock write failed: ");
          exit(3);
	}		
	if(pc_req_result==BM_NO_DONUTS)
	{
		printf("\n BM NO DONUTS");					
	}
	else //if(pc_req_result == BM_REQUEST_SUCCESS)
	{			
						
		if(read(conn1.inet_sock, &bm_req,sizeof(donut_request)) == -1)
		{
	          perror("inet_sock write failed: ");
	          exit(3);
		}					
		if(read(conn1.inet_sock, &req_complete,sizeof(int)) == -1)
		{
	          perror("inet_sock write failed: ");
	          exit(3);
		}
		printf("\n CONSUMED DONUT %d %d %d %d ",bm_req.donutType, bm_req.donutValue, bm_req.nodeNumber, bm_req.timeStamp);		
	}
	return 1;
}

void    *consumer ( void *arg )
{
	printf("Inside the consumer %d",*(int *)arg);
	int	 j,id;
	int count;
	unsigned short 	xsub[3];
	struct timeval 	randtime;
	tMsg msg;
	donut_request myDonut;
	prodOrConsumeRequest pc_req;
	struct_Mutex *tempMutex;
	int tempi,tempj,countDonuts;
	donut_request consumedDonuts[DOZEN];

	FILE  *handler;
	char filename[20];
	/*#ifdef WRITE_LOG*/
	id= (systemNodeNumber*10+(( *(int*)arg)-(numberOfNodes+NUMPRODUCERS)));

	sprintf(filename, "consumerlog%d.txt",id);
	countDonuts = 0;

	gettimeofday ( &randtime, ( struct timezone * ) 0 );	
	xsub[0]=(unsigned short)randtime.tv_sec;
	xsub[1]=(unsigned short)randtime.tv_sec>>16;
	xsub[2]=(unsigned short)(pthread_self);
	
	int productionCount=0;
	count=0;
	
	printf("\n\t Consumer- Total Number of donuts consumed %d",(NUM_OF_DOZENS*DOZEN));
	countDonuts = 0;
	
	while (count<(NUM_OF_DOZENS*DOZEN))
	{
		count++;
		productionCount++;
		//get the random number generator
		j = nrand48 ( xsub ) & 3;
	 	pthread_mutex_lock(&m_newMsg_nc);
	 	while(newMsg_raiseflag==1)
	 	{
	 		printf("\nConsumer-Thread Wait for NC - to read the new Message\n");
	 		pthread_cond_wait(&cond_m_newMsg_nc,&m_newMsg_nc);		 		
	 	}	 	
	 	msg.nodeNumber=systemNodeNumber;	 	
	 	msg.threadId=*(int *)arg;
	 	nc_newMsg=msg;
	 	printf("\n Consumer %d  of threadId %d -wants to consume",msg.nodeNumber,msg.threadId);
	 	newMsg_raiseflag=1;	 	
	 	pthread_mutex_unlock(&m_newMsg_nc);
	 	pthread_cond_signal(&cond_m_newMsg_nc);
	 	
		pthread_mutex_lock(&m_criticalSection_threadId);
		while(criticalSection_threadId!=*(int *)arg)
		{
			printf("\nThread %d waiting for its critical section ",*(int *)arg);
			pthread_cond_wait(&cond_criticalSection_threadId,&m_criticalSection_threadId);
		}		
		myDonut.donutType=j;
		myDonut.donutValue=serialCounter[j];
		serialCounter[j]++;
		myDonut.prodThdId=*(int *)arg;
		myDonut.nodeNumber=systemNodeNumber;
		myDonut.timeStamp=criticalSection_timeStamp;
		if(consumeRemoteDonut(myDonut)==1)
		{
			printf("Thread %d Consumer Completed",*(int *)arg);
			


			if(countDonuts<DOZEN)
			{ 
				consumedDonuts[countDonuts]=myDonut;
				countDonuts++;
			}
			else
			{	
				
				handler = fopen(filename, "a");
				if(handler == NULL)
				{
					printf("File open ERROR...No LOG File will be created...\n");
				}
				fprintf(handler,"\n\t\t\t printing after 12 Donuts were Consumed\n ");
				fprintf(handler,"=====================================================================\n");
				for(tempj=0;tempj<4;tempj++)
				{
					fprintf(handler,"TYPE OF DONUT %s ||",donutName[tempj]);
					for(tempi=0;tempi<DOZEN;tempi++)
					{
						if(consumedDonuts[tempi].donutType==tempj)
						{
							fprintf(handler,"    ||\t %d",consumedDonuts[tempi].donutValue);

						}
					}
					fprintf(handler,"\n");
				}
				fprintf(handler,"\n========================================================================\n");
				countDonuts=0;
				fclose(handler);
		
			//close file
			}
			
		}
		else
		{
			serialCounter[j]--;
			printf("Thread %d Consumer Failed",*(int *)arg);
		}		
		criticalSection_threadId=0;
		pthread_cond_signal(&cond_nc_producer);
		pthread_mutex_unlock(&m_criticalSection_threadId);		
	}			
}

void init()
{
	int i;
	pthread_mutex_init(&allNodes_status,NULL);
	pthread_mutex_init(&gotMessage,NULL);
	pthread_mutex_init(&m_newMsg_nc,NULL);
	pthread_mutex_init(&m_noMsg_srv_thds,NULL);
	pthread_mutex_init(&m_print,NULL);
	pthread_mutex_init(&m_nc_producer,NULL);
		
	pthread_cond_init(&cond_nc_producer,NULL);
	pthread_cond_init(&cond_allNodes_status,NULL);
	pthread_cond_init(&cond_gotMessage,NULL);
	pthread_cond_init(&cond_m_newMsg_nc,NULL);
	pthread_cond_init(&cond_m_noMsg_srv_thds,NULL);
	pthread_cond_init(&cond_print,NULL);		
	
	for(i=0;i<NUMFLAVORS;i++)
	{
		serialCounter[i]=1;		
	}
	
	pthread_attr_init ( &thread_attr );
	pthread_attr_setinheritsched ( &thread_attr,PTHREAD_INHERIT_SCHED );
		
	#ifdef  GLOBAL
		sched_struct.sched_priority = sched_get_priority_max(SCHED_OTHER);
		pthread_attr_setinheritsched ( &thread_attr,
				PTHREAD_EXPLICIT_SCHED );
		pthread_attr_setschedpolicy ( &thread_attr, SCHED_OTHER );
		pthread_attr_setschedparam ( &thread_attr, &sched_struct );  
		pthread_attr_setscope ( &thread_attr,
				PTHREAD_SCOPE_SYSTEM );
	#endif	
		
}

void startAllThreads()
{
	int i,arg_array [20];	
	init();
	criticalSection_threadId=-1;
	
	for(i=0;i<1+numberOfNodes+NUMPRODUCERS+NUMCONSUMERS;i++)
	{
		thread_id[i]=i;
		void * temp=&i;
		arg_array [i]=i;
						
		if(i==0)
		{
			if ( pthread_create (&thread_id[i], &thread_attr,thd_startNodeController, NULL ) != 0 )
			{
				printf( "pthread_create failed " );
				//exit ( 3 );
			}
		}
		else if(i>0&&i<numberOfNodes+1)
		{
			//if ( pthread_create (&thread_id[i], &thread_attr,thd_serverListen,NULL) != 0 )
			if(i!=systemNodeNumber)
			{
				if ( pthread_create ( &thread_id[i], &thread_attr,
						thd_serverListen, ( void * )&arg_array [i]) != 0 )
				{
					printf ( "pthread_create failed " );
					//exit ( 3 );
				}
				printf(" int i %d",i);
			}
		}
		else if(i>numberOfNodes && i<(numberOfNodes+1+NUMPRODUCERS))
		{
			if ( pthread_create ( &thread_id[i], &thread_attr,
							producer, ( void * )&arg_array [i]) != 0 )
			{
				printf ( "pthread_create failed" );
				exit ( 3 );
			}			
		}
		else if(i>(numberOfNodes+NUMPRODUCERS)&&i<(numberOfNodes+1+NUMPRODUCERS+NUMCONSUMERS))
		{
			if ( pthread_create ( &thread_id[i], &thread_attr,
										consumer, ( void * )&arg_array [i]) != 0 )
			{
				printf ( "pthread_create failed" );
				exit ( 3 );
			}	
		}		
	}
	 
	for ( i = numberOfNodes+1;i<(numberOfNodes+1+NUMPRODUCERS);i++)
			pthread_join ( thread_id [i], NULL ); 	
	printf("\n ==================All producers finished the production for this node ==============");
	while(2>1)
	{		
	}
		
}


int main()
{
	int count=0;
	int choice;
	
	connectObj conn1,conn2;
	tMsg msg;
	int tmp_port;
	donut_request bm_req;
	prodOrConsumeRequest pc_req;
	int pc_req_result,req_complete;
	numberOfNodes=3;
	newMsg_raiseflag=0;
	// remove
	int tmpValue,tmpFlavor;
	
	printf("\nEnter Node Number\n");
	scanf("%d",&systemNodeNumber);
	startAllThreads();
	printf("End of Main");
}

