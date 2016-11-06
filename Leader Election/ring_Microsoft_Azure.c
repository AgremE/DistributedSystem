#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <memory.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <mpi.h>
#include <ctype.h>

#define DEBUG_STATEMENT(X)
//#define DEBUG_STATEMENT(X) X

#define BUFFER_SIZE 1024

enum message_type
{
	REQUEST,
	RESPONSE,


	TYPE_MAX,
};

const char* TYPE_NAME[TYPE_MAX] =
{
		"REQUEST",
		"RESPONSE",
};

struct mpi_message
{
	int type;
	int sender;
	int leader;
	int terminated;
	char sender_uuid[37];
	char max_uuid[37];
};
struct thread_arg
{
	int rank;
	int size;
	char my_uuid[37];
	int loop_limit;

	char max_uuid[37];
	int leader;
	int terminated;
	int type;
};

int pack_mpi_message(const struct mpi_message* msg, void* buffer, int bufsize)
{
	int pos = 0;
	memset(buffer, 0, bufsize);

	MPI_Pack(&msg->type, 1, MPI_INT, buffer, bufsize, &pos, MPI_COMM_WORLD);
	MPI_Pack(&msg->sender, 1, MPI_INT, buffer, bufsize, &pos, MPI_COMM_WORLD);
	MPI_Pack(&msg->sender_uuid, 37, MPI_CHAR, buffer, bufsize, &pos, MPI_COMM_WORLD);
	MPI_Pack(&msg->max_uuid, 37, MPI_CHAR, buffer, bufsize, &pos, MPI_COMM_WORLD);
	MPI_Pack(&msg->leader, 1, MPI_INT, buffer, bufsize, &pos, MPI_COMM_WORLD);
	MPI_Pack(&msg->terminated, 1, MPI_INT, buffer, bufsize, &pos, MPI_COMM_WORLD);

	return pos;
}

void unpack_mpi_message(struct mpi_message* msg, const void* buffer, int bufsize)
{
	int pos = 0;
	memset(msg, 0, sizeof(struct mpi_message));

	MPI_Unpack(buffer, bufsize, &pos, &msg->type, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Unpack(buffer, bufsize, &pos, &msg->sender, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Unpack(buffer, bufsize, &pos, &msg->sender_uuid, 37, MPI_CHAR, MPI_COMM_WORLD);
	MPI_Unpack(buffer, bufsize, &pos, &msg->max_uuid, 37, MPI_CHAR, MPI_COMM_WORLD);
	MPI_Unpack(buffer, bufsize, &pos, &msg->leader, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Unpack(buffer, bufsize, &pos, &msg->terminated, 1, MPI_INT, MPI_COMM_WORLD);
}


void* sender(void* arg)
{
	struct thread_arg *arg_ = (struct thread_arg*)arg;
	int rank = arg_->rank;
	int size = arg_->size;
	char my_uuid[37];
	char max_uuid[37];
	strcpy(my_uuid,arg_->my_uuid);
	strcpy(max_uuid,arg_->max_uuid);
	int loop_limit = arg_->loop_limit;
	int leader = arg_->leader;
	int terminated = arg_->terminated;

	int dest = (rank + 1) % size;

	DEBUG_STATEMENT(printf("snd thread input: rank:%d size:%d, pri:%d, limit:%d, dest:%d\n", rank, size, my_priority, loop_limit, dest));

	struct mpi_message message;
	memset(&message, 0, sizeof(message));

	char buffer[BUFFER_SIZE];

	int loop_count = 0;
	while(loop_limit == -1 || loop_count < loop_limit)
	{
		message.sender = rank;
		strcpy(message.sender_uuid,my_uuid);
		strcpy(message.max_uuid,max_uuid);
		message.leader = leader;
		message.terminated = terminated;
		int pos = pack_mpi_message(&message, buffer, sizeof(buffer));
		DEBUG_STATEMENT(printf("message packed send %d\n", rank));
		//printf("Sender UUID: %s \n",my_uuid );  // Print user's home directory.
		MPI_Send(buffer, pos, MPI_PACKED, dest, 0, MPI_COMM_WORLD);
		loop_count ++;

	}

	return 0;
}

void* receiver(void* arg)
{
	struct thread_arg *arg_ = (struct thread_arg*)arg;

	int rank = arg_->rank;
	DEBUG_STATEMENT(int size = arg_->size);
	//Recieve Message from the sender
	char my_uuid[37];
	strcpy(my_uuid, arg_->my_uuid);
	int loop_limit = arg_->loop_limit;
	int size = arg_->size;
	char pring_uuid[37];

	int dest = (rank + 1) % size;

	DEBUG_STATEMENT(printf("recv thread input: rank:%d size:%d, pri:%d, limit:%d\n", rank, size, my_priority, loop_limit));

	struct mpi_message message;
	memset(&message, 0, sizeof(message));
	char recv_buffer[BUFFER_SIZE];
	char send_buffer[BUFFER_SIZE];

	int loop_count = 0;
	// Change it to send only when the max update 
	while(loop_limit == -1 || loop_count < loop_limit)
	{
		DEBUG_STATEMENT(printf("start recv %d\n", rank));

		if(arg_->terminated == 0){
			MPI_Recv(recv_buffer, sizeof(recv_buffer), MPI_PACKED, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			DEBUG_STATEMENT(printf("end recv %d\n", rank));
			unpack_mpi_message(&message, recv_buffer, sizeof(recv_buffer));
			}	

		//printf("\n");
		//printf("In comming message: Send to %d, received TYPE[%s], SENDER[%d], SENDER_PRIORITY[%d], Message Highest Priority: %d, Terminated[%d] \n"
		//					, rank, TYPE_NAME[message.type], message.sender, message.sender_priority,message.max_value,message.terminated);
		//printf("Message Highest Priority: %d\n", message.max_value);
    	
		arg_->terminated = message.terminated;
		strcpy(arg_->max_uuid,message.max_uuid);
		arg_->leader = message.leader;

		//uuid_unparse(message.max_uuid,pring_uuid);
		//printf("Message Max UUID: %s\n", message.max_uuid);
		
		int pos = pack_mpi_message(&message, send_buffer, sizeof(send_buffer));
		DEBUG_STATEMENT(printf("message packed recv %d\n", rank));
		//printf("Message Sending: Send from %d, received TYPE[%s], SENDER[%d], SENDER_PRIORITY[%d], Message Highest Priority: %d, Terminated[%d] \n"
		//					, rank, TYPE_NAME[message.type], message.sender, message.sender_priority,message.max_value,message.terminated);
		//MPI_Send(send_buffer, pos, MPI_PACKED, dest, 0, MPI_COMM_WORLD);
		//printf("Terminated Condition of process %d: %d\n", arg_->rank, arg_->terminated);
		//printf("I'm %d, highest_priority [%d]\n",rank,arg_->max_value);
		//printf("Loop: %d\n",loop_count );
	loop_count++;
}

	return 0;
}
// Using tag in the message for helping the leader election
int main(int argc, char** argv)
{
	int rank, size;
	int leader;
	int loopCount=0;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int nameleg;
	int messNum;
	MPI_Init (&argc, &argv);	/* starts MPI */
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */
	MPI_Get_processor_name(processor_name, &nameleg);// get name of the processor

	// Should try to do a while loop so I can keep sending the message out until I go the terminate message
	FILE *fp;
	char path[46];
  	char uuid[37];
	char print_uuid[37];
	char print_seond_uuid[37];
	char uuid_leader[37];
	char my_uuid[37];
	//uuid_generate_time_safe(my_uuid);
	 /* Open the command for reading. */
	/* Read the output a line at a time - output it. */
	
	fp = popen("sudo dmidecode | grep UUID", "r");
	if (fp == NULL) {
	  printf("Failed to run command\n" );
	  exit(1);
	}

	while (fgets(path, sizeof(path)-1, fp) != NULL) { 

	 	int l = strlen(path);
	    memcpy(my_uuid, path + 7, l - 7 );
	    int counter = 0;
	    //printf("UUID: ");
	    while(my_uuid[counter]){
	      my_uuid[counter] = tolower(my_uuid[counter]);
	      counter++;
	    }

	  }
	printf("Process : %d UUID: %s", rank, my_uuid);
	printf("\n");
	pclose(fp);
	
	/* close */
	//srandom(time(0) + rank);
	//int my_priority = random();
	

	pthread_t send_thread, recv_thread;


	struct thread_arg *arg1 = malloc(sizeof(struct thread_arg));

	strcpy(arg1->my_uuid , my_uuid);
	arg1->rank = rank;
	arg1->size = size;
	arg1->leader = -1;
	arg1->loop_limit = 1;
	arg1->terminated = 0;
	arg1->type = REQUEST;
	if(strcmp(my_uuid , arg1->max_uuid) > 0){
			
			strcpy(arg1->max_uuid,my_uuid);
		
		}
	//Recieve Priority from other thread of the other process

	struct thread_arg *arg2 = malloc(sizeof(struct thread_arg));
	
	strcpy(arg2->my_uuid,my_uuid);
	arg2->rank = rank;
	arg2->size = size;
	arg2->leader = -1;
	arg2->loop_limit = 1;
	arg2->terminated = 0;
	arg2->type = REQUEST;

	if(strcmp(my_uuid,arg2->max_uuid)){
			
			strcpy(arg2->max_uuid,my_uuid);
		
		}
	//sleep(2);

	while(arg1->terminated != 1)
	{
	
		sleep(1);
		
		if(size == 1){
			break;
		}
		else{
			if(arg2->terminated == 1){
				arg1->terminated = 1;
				arg1->leader = arg2->leader;
				strcpy(arg1->max_uuid,arg2->max_uuid);

			}	
			else if(strcmp(arg1->max_uuid, arg2->max_uuid) < 0){

				strcpy(arg1->max_uuid,arg2->max_uuid);
				arg1->leader = arg2->leader;
				arg1->terminated = 0;
			}
			else if(strcmp(arg1->max_uuid,arg2->max_uuid)==0){

				if(arg2->leader == arg1->rank){
					arg1->terminated = 1;
				}
				else{
					arg1->leader = rank;
					arg1->terminated = 0;
				}


			}
		}
		//printf("arg1->max_value %d and arg2->max_value: %d\n", arg1->max_value, arg2->max_value);

		//loop_countZero: loopCount++;
		
		pthread_create(&send_thread, 0, sender, arg1);
		messNum++;
		//printf("%s sent the message \n", processor_name);		
		pthread_create(&recv_thread, 0, receiver, arg2);
		//printf("%s received the message \n", processor_name);  // Print user's home directory.

		DEBUG_STATEMENT(printf("thread created %d\n", rank));

	}
	
	printf("Leader UUID: %s Leader Node ID: %d Node ID: %d Computer Node Identity: %s\n", arg1->max_uuid, arg1->leader, rank, processor_name);
	printf("Number of message: %d\n", messNum);
	void* ret;
	
	if(size != 1){
		pthread_join(send_thread, &ret);
		pthread_join(recv_thread, &ret);
	}
	free(arg1);
	free(arg2);

	fflush(0);
	MPI_Finalize();
	return 0;
}
