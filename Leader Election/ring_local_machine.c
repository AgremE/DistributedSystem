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
	int sender_priority;

	int max_value;
	int leader;
	int terminated;
};

int pack_mpi_message(const struct mpi_message* msg, void* buffer, int bufsize)
{
	int pos = 0;
	memset(buffer, 0, bufsize);

	MPI_Pack(&msg->type, 1, MPI_INT, buffer, bufsize, &pos, MPI_COMM_WORLD);
	MPI_Pack(&msg->sender, 1, MPI_INT, buffer, bufsize, &pos, MPI_COMM_WORLD);
	MPI_Pack(&msg->sender_priority, 1, MPI_INT, buffer, bufsize, &pos, MPI_COMM_WORLD);
	MPI_Pack(&msg->max_value, 1, MPI_INT, buffer, bufsize, &pos, MPI_COMM_WORLD);
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
	MPI_Unpack(buffer, bufsize, &pos, &msg->sender_priority, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Unpack(buffer, bufsize, &pos, &msg->max_value, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Unpack(buffer, bufsize, &pos, &msg->leader, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Unpack(buffer, bufsize, &pos, &msg->terminated, 1, MPI_INT, MPI_COMM_WORLD);
}

struct thread_arg
{
	int rank;
	int size;
	int my_priority;
	int loop_limit;

	int max_value;
	int leader;
	int terminated;
	int true_leader;
	int type;
	int tmp_max;
};

void* sender(void* arg)
{
	struct thread_arg *arg_ = (struct thread_arg*)arg;
	int rank = arg_->rank;
	int size = arg_->size;
	int my_priority = arg_->my_priority;
	int loop_limit = arg_->loop_limit;
	int max_value = arg_->max_value;
	int leader = arg_->leader;
	int terminated = arg_->terminated;
	int tmp_max = arg_->tmp_max;

	int dest = (rank + 1) % size;

	DEBUG_STATEMENT(printf("snd thread input: rank:%d size:%d, pri:%d, limit:%d, dest:%d\n", rank, size, my_priority, loop_limit, dest));

	struct mpi_message message;
	memset(&message, 0, sizeof(message));

	char buffer[BUFFER_SIZE];

	int loop_count = 0;
	while(loop_limit == -1 || loop_count < loop_limit)
	{
		message.sender = rank;
		message.sender_priority = my_priority;
		message.max_value = max_value;
		message.leader = leader;
		message.terminated = terminated;
		int pos = pack_mpi_message(&message, buffer, sizeof(buffer));
		DEBUG_STATEMENT(printf("message packed send %d\n", rank));
		//printf("%s Sending the message out \n", getenv("USER"));  // Print user's home directory.
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
	int my_priority = arg_->my_priority;
	int loop_limit = arg_->loop_limit;
	int size = arg_->size;

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
		arg_->max_value = message.max_value;
		arg_->leader = message.leader;		
		
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
	
	srandom(time(0) + rank);
	int my_priority = random();
	printf("Hello world from process %d of %d, priority %d\n", rank, size, my_priority);

	pthread_t send_thread, recv_thread;


	struct thread_arg *arg1 = malloc(sizeof(struct thread_arg));

	arg1->my_priority = my_priority;
	arg1->rank = rank;
	arg1->size = size;
	arg1->leader = -1;
	arg1->loop_limit = 1;
	arg1->terminated = 0;
	arg1->max_value = -1;
	arg1->type = REQUEST;
	arg1->true_leader = -1;
	if(my_priority > arg1->max_value){
			
			arg1->max_value = my_priority;
		
		}
	//Recieve Priority from other thread of the other process

	struct thread_arg *arg2 = malloc(sizeof(struct thread_arg));
	
	arg2->my_priority = my_priority;
	arg2->rank = rank;
	arg2->size = size;
	arg2->leader = -1;
	arg2->loop_limit = 1;
	arg2->terminated = 0;
	arg2->max_value = -1;
	arg1->true_leader = -1;
	arg2->type = REQUEST;

	if(my_priority > arg2->max_value){
			
			arg2->max_value = my_priority;
		
		}
	
	sleep(2);

	while(arg1->terminated != 1)
	{
	
		sleep(1);
		int max = arg1->max_value;
		
		if(size == 1 ){
			break;
		}else{
			if(arg2->terminated == 1){
				arg1->terminated = 1;
				arg1->leader = arg2->leader;
				arg1->max_value = arg2->max_value;

			}	
			else if(arg1->max_value < arg2->max_value){

				arg1->max_value = arg2->max_value;
				arg1->leader = arg2->leader;
				arg1->terminated = 0;
			}
			else if(arg1->max_value == arg2->max_value){

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

	printf("Leader Priority: %d Leader Node ID: %d Node ID: %d Computer Node Identity: %s\n", arg1->max_value, arg1->leader, rank, processor_name);
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
