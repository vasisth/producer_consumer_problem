#ifndef COMMON_H_
#define COMMON_H_

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <malloc.h>
#include "pthread.h"
#include "stdio.h"
#include "sys/time.h"

#define         NUMFLAVORS       	4
#define         NUMSLOTS        	100
#define         NUMPRODUCERS     	1
#define         NUMCONSUMERS     	1
#define 		NUM_OF_DOZENS		3
#define 		DOZEN			12
#define 		BM_PRODUCE_REQUEST 	0
#define 		BM_CONSUME_REQUEST 	1
#define 		BM_INVALID_REQUEST 	6
#define 		BM_REQUEST_SUCCESS	9
#define 		BM_REQUEST_FAIL		8
#define 		BM_SPACE_AVLBL		7
#define			BM_NO_SPACE		11
#define			BM_DONUTS_AVLBL		17
#define			BM_NO_DONUTS		21
#define			BM_NODE_INDEX		4

typedef struct
{
	int donutType;
	int donutValue;
	int bM_donutValue;
	int nodeNumber;
	int prodThdId;
	int timeStamp;
}donut_request;

typedef struct 
{
	int prodOrConsume;
	int flavor;
	int nodeNumber;
	int threadId;
	int ipAddress;
}prodOrConsumeRequest;

typedef struct  {
	donut_request    donut[NUMFLAVORS][NUMSLOTS];
	int 	producerIds[NUMFLAVORS][NUMSLOTS];
	int 	threadId;
	int     outptr[NUMFLAVORS];
	int		in_ptr [NUMFLAVORS];	
	int		serialCounter[NUMFLAVORS];
}donut_ring ;

typedef struct 
{
	pthread_mutex_t m_currMutex;	
	pthread_cond_t	cond_currMutex;	
}struct_Mutex;

typedef struct 
{
	int nodeNumber;	
	int timeStamp;
	int threadId;
	struct_Mutex *mutexPtr;	
}tMsg;

typedef struct
{
	tMsg * msg[3];
	int totalResponses;
	struct struct_cs_reqNode * nextNode;
	
}struct_cs_reqNode;



typedef struct
{
	int inet_sock;
	struct sockaddr_in *inet_telnum;
	int *fromlen;	
}connectObj;
typedef struct
{
	
}nodeConfiguration;
static char hostNames[5][10]={"dummy","localhost","localhost","localhost","localhost"};
//static char hostNames[5][10]={"dummy","91308-01","91308-02","91308-03","91308-04"};
//static int 	portNumber[5]={0,21000,22000,23000,25000};
static int      portNumber[5]={0,35136,35226,36126,36135};


static char donutName[4][10]={"PLAIN    ","JELLY    ","COCONUT  ","HONEY DIP",};


#endif /*COMMON_H_*/
