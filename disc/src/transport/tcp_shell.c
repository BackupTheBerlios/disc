
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "../list.h"
#include "../dprint.h"
#include "../aaa_lock.h"
#include "../locking.h"
#include "ip_addr.h"
#include "peer.h"
#include "tcp_accept.h"
#include "tcp_receive.h"
#include "tcp_shell.h"


#define ACCEPT_THREAD_ID      0
#define RECEIVE_THREAD_ID(_n) (1+(_n))


/* array with all the threads created by the tcp_shell */
static struct thread_info  *tinfo;
static unsigned int        nr_recv_threads;
/* linked list with the receiving threads ordered by load */
static struct list_head    rcv_thread_list;
static gen_lock_t          *list_mutex;


#define get_payload( _pos ) \
		list_entry( (_pos), struct thread_info, tl )



int init_tcp_shell( unsigned int nr_receivers)
{
	unsigned int i;

	tinfo = (struct thread_info*)shm_malloc
		( (nr_receivers+1)*sizeof(struct thread_info) );
	if (!tinfo) {
		LOG(L_ERR,"ERRROR:init_tcp_shell: no more free memory\n");
		goto error;
	}
	memset(tinfo, 0, sizeof((nr_receivers+1)*sizeof(struct thread_info)));
	nr_recv_threads = nr_receivers;

	/* prepare the list for receive threads */
	INIT_LIST_HEAD(  &rcv_thread_list );
	list_mutex = create_locks( 1 );
	if (!list_mutex) {
		LOG(L_ERR,"ERROR:init_tcp_shell: cannot create mutex!\n");
		goto error;
	}

	/* build the threads */
	/* accept thread */
	if (pipe( tinfo[ACCEPT_THREAD_ID].cmd_pipe )!=0) {
		LOG(L_ERR,"ERROR:init_tcp_shell: cannot create pipe for accept "
			"thread : %s\n", strerror(errno));
		goto error;
	}
	if (pthread_create( &tinfo[ACCEPT_THREAD_ID].tid, 0/*&attr*/,
	&do_accept, &tinfo[ACCEPT_THREAD_ID])!=0) {
		LOG(L_ERR,"ERROR:init_tcp_shell: cannot create dispatcher thread\n");
		goto error;
	}

	/* receive threads */
	for(i=0; i<nr_recv_threads; i++) {
		if (pipe( tinfo[RECEIVE_THREAD_ID(i)].cmd_pipe )!=0) {
			LOG(L_ERR,"ERROR:init_tcp_shell: cannot create pipe for receive "
				"thread %d : %s\n", i,strerror(errno));
			goto error;
		}
		if (pthread_create( &tinfo[RECEIVE_THREAD_ID(i)].tid, 0/*&attr*/ ,
		&do_receive, &tinfo[RECEIVE_THREAD_ID(i)])!=0 ) {
			LOG(L_ERR,"ERROR:init_tcp_shell: cannot create receive thread\n");
			goto error;
		}
		list_add_tail( &(tinfo[RECEIVE_THREAD_ID(i)].tl), &rcv_thread_list);
	}

	LOG(L_INFO,"INFO:init_tcp_shell: tcp shell started\n");
	return 1;
error:
	terminate_tcp_shell();
	return -1;
}



void terminate_tcp_shell()
{
	struct command cmd;
	int i;

	/* build a  SHUTDOWN command */
	memset( &cmd, 0, COMMAND_SIZE);
	cmd.code = SHUTDOWN_CMD;

	/* send it to the accept thread */
	write( tinfo[ACCEPT_THREAD_ID].cmd_pipe[1], &cmd, COMMAND_SIZE);

	/* ... and to the receiver threads */
	for(i=0; i<nr_recv_threads; i++)
		write( tinfo[RECEIVE_THREAD_ID(i)].cmd_pipe[1] , &cmd, COMMAND_SIZE);

	/* now wait for them to end */
	/* accept thread */
	pthread_join( tinfo[ACCEPT_THREAD_ID].tid, 0);
	LOG(L_INFO,"INFO:terminate_tcp_shell: accept thread terminated\n");
	/* receive threads */
	for(i=0; i<nr_recv_threads; i++) {
		pthread_join( tinfo[RECEIVE_THREAD_ID(i)].tid, 0);
		LOG(L_INFO,"INFO:terminate_tcp_shell: receive thread %d "
			"terminated\n",i);
	}

	/* destroy the lock list */
	destroy_locks( list_mutex, 1);

	LOG(L_INFO,"INFO:terminate_tcp_shell: tcp shell stoped\n");
}



void start_tcp_accept()
{
	struct command cmd;

	/* build a START command */
	memset( &cmd, 0, COMMAND_SIZE);
	cmd.code = START_ACCEPT_CMD;

	/* start the accept thread */
	write( tinfo[ACCEPT_THREAD_ID].cmd_pipe[1], &cmd, COMMAND_SIZE );

}



int get_new_receive_thread()
{
	struct thread_info *ti;
	struct list_head   *pos;

	/* lock the list */
	lock_get( list_mutex );

	/* get the first element from list */
	ti = get_payload( rcv_thread_list.next );
	/* increase its load */
	ti->load++;

	if ( ti->tl.next!=&ti->tl && get_payload(ti->tl.next)->load<ti->load ){
		/* remove the first element */
		list_del( rcv_thread_list.next );
		/* put it back in list into the corect position */
		list_for_each( pos, &rcv_thread_list ) {
			if ( get_payload(pos)->load==ti->load ) {
				list_add_tail( &(ti->tl), pos);
				break;
			}
		/* add at the end */
		list_add_tail( &(ti->tl), &rcv_thread_list);
		}
	}
	/* unlock the list */
	lock_release( list_mutex );

	return ti->cmd_pipe[1];
}



void release_peer_thread()
{
	/* lock the list */
	lock_get( list_mutex );

	//.......................

	/* unlock the list */
	lock_release( list_mutex );
}



#undef get_payload


