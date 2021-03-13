#include "distcommon.h"
#include "pthread.h"

donut_ring sharedRing;
int accessDonuts[NUMFLAVORS];
donut_request mr;

pthread_t		thread_id [10], sig_wait_id;
pthread_mutex_t 	m_flvAccess[NUMFLAVORS];
pthread_cond_t	cond_flvAccess[NUMFLAVORS];
pthread_attr_t	thread_attr;

connectObj serverListen(int portNumber);

void initialize()
{
	int i;	
	for(i=0;i<NUMFLAVORS;i++)
	{
		pthread_mutex_init(&m_flvAccess[i],NULL);
		pthread_cond_init(&cond_flvAccess[i],NULL);
		accessDonuts[i]=0;
		sharedRing.outptr[i]=0;
		sharedRing.in_ptr[i]=0;
		sharedRing.serialCounter[i]=1;			
	}		
	pthread_attr_init ( &thread_attr);
	pthread_attr_setinheritsched ( &thread_attr,PTHREAD_INHERIT_SCHED);
		
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

void logRequest(prodOrConsumeRequest req)
{

	printf("\nGot Request:Node :%d :Flavor %d  ",req.nodeNumber,req.flavor);
}
void printSharedDonut()
{
	int i,j,done;

	printf("\n");
	for(i=0;i<NUMFLAVORS;i++)
	{		
		for(j=sharedRing.outptr[i];j!=sharedRing.in_ptr[i];)
		{

		printf("\tNODE ID : %d :: DONUT : %s :: VALUE : %d\n",sharedRing.donut[i][j].nodeNumber, donutName[i],sharedRing.donut[i][j].donutValue);

			
			fflush(stdout);
			if((j+1)==NUMSLOTS)
			{
				j=0;
			}
			else
			{
				j++;
			}
		}		
	}		
}
int checkSpace(int flavor)
{
	int nextIndex;	
	nextIndex=(sharedRing.in_ptr[flavor]+1)%NUMSLOTS;	
	if(nextIndex!=sharedRing.outptr[flavor])
	{

		printf("\t\t\tSpace available...\n");
		return BM_SPACE_AVLBL;
	}
	else 
	{

		printf("\t\t\tNO SPACE...");
		return BM_NO_SPACE;
	}
}
int isDonutEmpty(int flavor)
{
	if(sharedRing.in_ptr[flavor]==sharedRing.outptr[flavor])
	{
		printf("\t\t\tNO DONUTS...");
		return BM_NO_DONUTS;
	}
	else
	{
		printf("\t\t\tDonuts available...\n");
		return BM_DONUTS_AVLBL;
	}
}

void *thd_processRequest(void *arg)
{
	int tempFlv;
	int requestResult;
	donut_request donutObj;
	prodOrConsumeRequest reqType;	
	fflush(stdout);
	if(read(*(int *)arg,&reqType,sizeof(prodOrConsumeRequest))==-1)
	{
		perror("inet_sock write failed: ");
		exit(3);
	}	
	if(reqType.prodOrConsume==BM_PRODUCE_REQUEST)
	{		

		printf("\n\tProduce Request For Flavor :: %d From Node :: %d\n\n",reqType.flavor,reqType.nodeNumber);	
		tempFlv=reqType.flavor;
		
		if(checkSpace(tempFlv)==BM_NO_SPACE)
		{
			requestResult=BM_NO_SPACE;			
			if(write(*(int *)arg,&requestResult,sizeof(int))==-1)
			{
				perror("inet_sock write failed: ");
				exit(3);
			}
		}
		else
		{
			requestResult=BM_SPACE_AVLBL;
			if(write(*(int *)arg,&requestResult,sizeof(int))==-1)
			{
				perror("inet_sock write failed: ");
				exit(3);
			}			
			if(read(*(int *)arg,&donutObj,sizeof(donut_request))==-1)
			{
				perror("inet_sock write failed: ");
			    exit(3);
			}
			else
			{	
				tempFlv=reqType.flavor;			
				donutObj.bM_donutValue=sharedRing.serialCounter[tempFlv];
				sharedRing.serialCounter[tempFlv]+=1;
				sharedRing.donut[tempFlv][sharedRing.in_ptr[tempFlv]]=donutObj;
				sharedRing.in_ptr[tempFlv]++;
				if(sharedRing.in_ptr[tempFlv]==NUMSLOTS)
				{
					sharedRing.in_ptr[tempFlv]=0;
				}
				requestResult=BM_REQUEST_SUCCESS;
				if(write(*(int *)arg, &requestResult,sizeof(int)) == -1)
				{
					perror("inet_sock write failed: ");
					exit(3);
				}				
				//accessDonuts[tempFlv]==0;
				printSharedDonut();
			}				
		}
	}
	else if(reqType.prodOrConsume==BM_CONSUME_REQUEST)
	{

		tempFlv=reqType.flavor;	
		printf("\n\tConsume Request For Flavor :: %d From Node :: %d\n\n", reqType.flavor,reqType.nodeNumber);
		
		if(isDonutEmpty(tempFlv)==BM_NO_DONUTS)
		{
			requestResult=BM_NO_DONUTS;
			if(write(*(int *)arg,&requestResult,sizeof(int))==-1)
			{
				perror("inet_sock write failed: ");
				exit(3);
			}
		}
		else
		{
						
			if(write(*(int *)arg,&requestResult,sizeof(int))==-1)
			{
				perror("inet_sock write failed: ");
				exit(3);
			}		
			tempFlv=reqType.flavor;			
			
			donutObj=sharedRing.donut[tempFlv][sharedRing.outptr[tempFlv]];
			sharedRing.outptr[tempFlv]++;
			if(sharedRing.outptr[tempFlv]==NUMSLOTS)
			{
				sharedRing.outptr[tempFlv]=0;
			}
			if(write(*(int *)arg,&donutObj,sizeof(donut_request))==-1)
			{
				perror("inet_sock write failed: ");
			    exit(3);
			}
			requestResult=BM_REQUEST_SUCCESS;
			if(write(*(int *)arg, &requestResult,sizeof(int)) == -1)
			{
				perror("inet_sock write failed: ");
				exit(3);
			}				
			printSharedDonut();
		}
	}
	close(*(int *)arg);
}

/*start of main*/
int  main()
{	
	int curr_soc,i;
	connectObj conn;	
	//accept the sockets and allot different threads to
	initialize();
	conn=serverListen(portNumber[BM_NODE_INDEX]);
	
	i=0;
	while(1)
	{
		printf("\n===============================================================================");
		printf("\n\t\t\tDonut Request number = %d",++i);
		printf("\n===============================================================================");
		fflush(stdout);
		curr_soc=acceptClient(conn);
		if(curr_soc==-1)
		{
			perror("Accept Failed");
			exit(2);
		}
		else
		{
			if ( pthread_create (&thread_id[i], &thread_attr,thd_processRequest,(void *)&curr_soc ) != 0 )
			{
				perror("Unauthorised process Requestble ");

			}
		}
		pthread_join(thread_id[i],NULL);
	}
	
}
