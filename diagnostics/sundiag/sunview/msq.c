#ifndef lint
static  char sccsid[] = "@(#)ATS msq.c 1.1 92/07/30 Copyright Sun Microsystems Inc.";
#endif

#define	NO_LWP	1
/* -JCH- the only difference between amp.c(ATS) and msq.c(Sundiag) */

/**
 * ATS Message Protocol (AMP):
 *
 * FUNCTION:
 *
 *   AMP manages all message traffic among ATS processes.  All in-coming
 *   messages are passed to the destination immediately.  All out-going
 *   messages are sent out immediately and they are also placed on the
 *   waiting-for-ack (wack) array.  An acknowledgement is required for
 *   all out-going messages (except AMP_ACK_MESSAGE).  If an acknowledgement
 *   is not received by the sender after a specified timeout, the message
 *   is retransmitted until a maximum transmission is reached.  After
 *   a maximum transmission (user defined), amp will execuate an error
 *   handler if there is any (user supplied).
 *
 *   AMP is capable of manages multiple thread processes.  Each thread
 *   process has its own amp resources.
 *   
 *   If you are not using the lwp (lightweight process library), you should
 *   define NO_LWP in the amp.c or at compile time.
 *
 *   You need to include "amp.h" file in your program.
 *
 * EXPORTED ROUTINES:
 *
 *   amp_initialize()       -  allocates amp resources, must be called first;
 *                             Return: 1 - success;
 *                                     0 - failed;
 *
 *   amp_exit()             -  exits from amp, must be called;
 *                             Return: 1 - success;
 *                                     0 - failed;
 *
 * Sends rpc message.
 *   amp_rpc_send(client, in, in_size, inproc, msg_cb)
 *     CLIENT *client;      -  destination of this message;
 *     char   *in;          -  data to be send to the receiver;
 *     int    in_size;      -  data size of in;
 *     xdrproc_t inproc;    -  xdr routine to be used to encode the data;
 *     S_AMP_MSG_CB *msg_cb -  it contains user defined number of 
 *                             retransmission, timeout for ack, 
 *                             error handler, etc.
 *
 *                             Return: 1 - success;
 *                                     0 - failed;
 *     Example:
 *
 *       f()
 *       {
 *          S_AMP_MSG_CB  msg_cb;
 *          int i, *index;
 *          void msg_timeout_handler();
 *
 *          i = 5;
 *          *index=1;
 *          msg_cb.max_wack = 60;      { wait 60 seconds for the ack }
 *          msg_cb.max_retransmit = 10; { only retransmit 10 times }
 *          msg_cb.action = msg_timeout_handler;{call action after max trans}
 *          msg_cb.data = (char *)(index); {user supplied data. }
 *          msg_cb.data_size = sizeof(int); {size of user supplied data }
 *          msg_cb.message_num = 100;   {message number to be transmitted}
 *          amp_rpc_send(client, &i, sizeof(int), xdr_int, &msg_cb);
 *       }
 *
 *       void msg_timeout_handler(ptr, data)
 *            S_AMP_MSG_ARRAY *ptr;
 *            char *data;
 *       {
 *            int index;
 * 
 *            index = *(int *)data;
 *            printf("Message: %d, Seq: %d timeout for index: %d\n",
 *	         ptr->msg_cb->message_num, ptr->rec->amp_msg->sequence_num,
 *	         index);
 *       }
 *
 *
 *   amp_send_ack(client, ack_msg_num, procnum, sequence_num)
 *     CLIENT *client;      -  destination of this message;
 *     u_long ack_msg_num;  -  acknowledgement message number;
 *     u_long procnum;      -  acknowledge message <procnum> and
 *     int    sequence_num; -  <sequence_num>.
 *
 *                             Return: 1 - success;
 *                                     0 - failed;
 *
 *   amp_transp_send_ack(client, ack_msg_num, rqstp, xport)
 *     CLIENT *client;      -  destination of this message;
 *     u_long ack_msg_num;  -  acknowledgement message number;
 *     struct svc_req *rqstp; - pointer to service request.  It contains
 *                              the message number to be acknowledged.
 *     XDR *xport;     -  transport pointer.  It contains the sequence
 *                             number of the message to be acknowledged.  
 *                             The sequence number MUST be the first field
 *                             in the data structure.
 *
 * Check message array.
 *   amp_elapsed_time()     -  determines whether or not any message need
 *                             to be retransmitted.
 *                             
 *                             Return: None;
 *
 * Check acknowledgement messages.  If the message is in the message
 *   array, it is deleted from the array.
 *
 *   amp_check_ack_msg(ack_rec)
 *     S_AMP_ACK *ack_rec;  -  contains the message number and the sequence
 *                             number of the message.
 *
 *   amp_p_ack_msg(xport)
 *     XDR *xport;     -  the transp structure contains message in the
 *                             ack_msg format.  Use xdr_amp_ack to decode
 *                             the structure.
 *
 * Check whether the amp message array is empty or not.
 *   amp_empty_array()      -  Return: 1 - Empty;  0 - Not Empty;
 *
 * Prints out all messages in the message array.
 *   amp_print_array()      -  print out messages in the current thread;
 *   amp_print_all_array()  -  print out all messages in all thread amp.
 *
 * AMP ack xdr routine.  Used to encode and decode amp ack message.
 *  The first field in the data structure is the sequence number and the 
 *  second is the message number.
 *
 *  xdr_amp_ack(xdrsp, rec)
 *
 * 
 * Set the message sequence number:
 *   amp_set_seq_num(seq)  
 *     int seq;             -  starts the message sequence number from <sq>
 *
 * Receive in-coming messages.  It is used for lwp thread.  For non-lwp,
 *   use select(...) to received any in coming messages.  If the message
 *   is an ack message, the ack message is processed and it is not passed
 *   to the application program.  The message array is checked to determine
 *   whether or not any message needs to be retransmitted.  The message is 
 *   passed to the application program, except for the ack message.
 * 
 *   Receive messages from all threads.
 *
 *   amp_msg_recv_all(sender, arg, argsize, res, ressize, timeout)
 *     thread_t *sender;
 *     caddr_t *arg;
 *     int *argsize;
 *     caddr_t *res;
 *     int *ressize;
 *     struct timeval *timeout;
 *
 *   Receive messages from the given thread.
 *     amp_msg_recv(sender, arg, argsize, res, ressize, timeout)
 *
 *                     Return: 0 - success; 
 *                            -1 - timeout;
 *
 * Flush the message array.
 *   amp_flush_msg_array()  -  flushs out the wack array;
 *                             Return: 1 - success;
 *                                     0 - failed;
*/
 
#include <stdio.h>
#include <sys/types.h>          /* for stat system call */
#include <sys/stat.h>           /* for stat system call */
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <rpc/rpc.h>

#ifndef NO_LWP
#include <lwp/lwp.h>
#include <lwp/stackdep.h>
#include <lwp/lwperror.h>
#endif

#ifdef SunOS4
#include <rpc/svc.h>
#endif

#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
    
#include "amp.h"

#ifdef NO_LWP 
#define SAME_KEY   100
#define SAME_INDEX 0
#endif


#ifndef NO_LWP
#define AMP_MAX_WACK_ARRAY_SIZE 100
#else
#define AMP_MAX_WACK_ARRAY_SIZE 1
#endif

#define AMP_MAX_ARRAY_SIZE 100
#define AMP_MAX_SEQ_NUM    9000
#define AMP_UDP_TIMEOUT        0

typedef struct amp_wack_a {
  int key;
  enum msg_array_stat stat;
  int sequence_num;
  int retransmit;
  int current_que_index;
  struct amp_msg_array msg_a[AMP_MAX_ARRAY_SIZE];
} S_AMP_WACK_A;

int amp_debug = 0;
struct amp_wack_a amp_wack_array[AMP_MAX_WACK_ARRAY_SIZE];


int xdr_amp_ack();
int xdr_amp_msg();
extern char *sys_errlist[];

/*
 * Empty the wack message array.
*/
amp_flush_msg_array()
{
  int i, id;

  if (amp_get_old_index(&id)) { /* thread id exist in array */
    for (i=0; i<AMP_MAX_ARRAY_SIZE; i++)
      amp_delete(id, i);
  }
}

/*
 * Allocates amp resources for the current process thread
*/
amp_initialize()
{
  int i, id, key;

  if (amp_get_new_index(&id, &key)) {
    amp_wack_array[id].key = key;
    amp_wack_array[id].stat = IN_USE;
    amp_wack_array[id].retransmit = 0;
    amp_wack_array[id].current_que_index = 0;
    for (i=0; i<AMP_MAX_ARRAY_SIZE; i++)
      amp_wack_array[id].msg_a[i].stat = EMPTY;
    return(1);
  }
}

/*
 * Deallocates amp resources of the current process thread
*/
amp_exit()
{
  int i, id;

  if (amp_get_old_index(&id)) {
    amp_wack_array[id].stat = EMPTY;
    
    for (i=0; i<AMP_MAX_ARRAY_SIZE; i++)
      amp_delete(id, i);
  }
}

/*
 * Get the wack array index for the current process thread
*/
static
amp_get_old_index(id)
int *id;
{
  int i, key;
#ifndef NO_LWP
  thread_t c;

  lwp_self(&c);
  key = c.thread_key;

  for (i=0; i<AMP_MAX_WACK_ARRAY_SIZE; i++)
    if (amp_wack_array[i].key == key) {
      *id = i;
      return(1);
    }
  return(0);
#else
  *id = SAME_INDEX;
  return(1);
#endif
}

/*
 * Assign a new wack array index for the current process thread
*/
static
amp_get_new_index(id, key)
int *id, *key;
{
  int i, temp = -1;
#ifndef NO_LWP
  thread_t c;

  lwp_self(&c);
  *key = c.thread_key;

  for (i=0; i<AMP_MAX_WACK_ARRAY_SIZE; i++)
    if (amp_wack_array[i].key == *key) {
      *id = i;
      return(1);
    }
    else if (amp_wack_array[i].stat == EMPTY)
      temp = i;
      
  if (temp == -1) { /* array is full */
    *id = -1;
    return(0);
  }
  *id = temp;
#else          /* doesn't use lightweight process */
  *key = SAME_KEY;
  *id = SAME_INDEX;
#endif

  return(1);
}

/*
 * Sends out an acknowledgement for message <procnum> and
 * sequence <sequence_num>.
*/
amp_send_ack(client, ack_msg_num, procnum, sequence_num)
CLIENT *client;
u_long ack_msg_num;
u_long procnum;
int sequence_num;
{
    struct amp_ack ack_struct;
    enum clnt_stat status;

    ack_struct.message_num = procnum;
    ack_struct.sequence_num = sequence_num;
    status = udpcall(client, ack_msg_num, xdr_amp_ack, &ack_struct,
		   xdr_void, NULL, AMP_UDP_TIMEOUT);
    if (status == RPC_SUCCESS || status == RPC_TIMEDOUT)
      return(1);
    else 
      return(0);
}


/*
 * Sends out an acknowledgement for message <procnum> and
 * sequence <sequence_num>.
*/
amp_transp_send_ack(client, ack_msg_num, rqstp, xport)
     CLIENT         *client;
     u_long         ack_msg_num;
     struct svc_req *rqstp;
     XDR            *xport;
{
    struct amp_ack ack_struct;
    enum clnt_stat status;
    int  seq;
    xdr_int(xport, &seq);
    ack_struct.message_num = rqstp->rq_proc;    
    ack_struct.sequence_num = seq;
    status = udpcall(client, ack_msg_num, xdr_amp_ack, &ack_struct,
		   xdr_void, NULL, AMP_UDP_TIMEOUT);
    if (status == RPC_SUCCESS || status == RPC_TIMEDOUT)
      return(1);
    else 
      return(0);
}

/*
 * Checks whether or not any message in the current thread
 * needs to retransmitted.
*/
void
amp_elapsed_time()
{
  struct timeval tp;
  struct timezone tzp;
  int    elapsed_time, i, id;
  S_AMP_MSG_CB *msg_cb;
  S_AMP_ACK ack_rec;

  if (! amp_get_old_index(&id)) {
    fprintf(stderr,"<***amp_elapsed_time> Unknown thread: %d\n", id);
    return;
  }

  gettimeofday(&tp, &tzp);
  for (i=0; i<AMP_MAX_ARRAY_SIZE; i++) {
    if ((amp_wack_array[id].msg_a[i].stat == IN_USE) && 
	(amp_wack_array[id].msg_a[i].rec != NULL) &&
	(amp_wack_array[id].msg_a[i].msg_cb != NULL)) {
      if ((elapsed_time = tp.tv_sec - 
	   amp_wack_array[id].msg_a[i].rec->timestamp)
	  >= amp_wack_array[id].msg_a[i].msg_cb->max_wack) {/*send msg */
	msg_cb = amp_wack_array[id].msg_a[i].msg_cb;
	if (amp_wack_array[id].msg_a[i].rec->transmit_num >= 
	    msg_cb->max_retransmit) {
	  /*
	   * AMP attempted to transmit the message MAX_RETRANSMIT times
	   * without any success.  The user supplied error hanler is
	   * called.  
	   * The message is then taking off the queue.
	   */
	  if (msg_cb->action != NULL) {
	    S_AMP_MSG_ARRAY *ptr;
	    
	    ptr = &amp_wack_array[id].msg_a[i];
	    (*msg_cb->action)(ptr, msg_cb->data);
	  }
	  if (amp_debug)
	    fprintf(stderr, 
	    "***Can't send Msg: %ld after %d tries. Remove from msg queue.\n",
		    amp_wack_array[id].msg_a[i].msg_cb->message_num,
		    amp_wack_array[id].msg_a[i].msg_cb->max_retransmit);
	  /*
	   * Take the message off the queue after maximum attempts.
	   */
	  amp_delete(id, i);
	  continue;
	}
	amp_wack_array[id].retransmit = 1;
	amp_wack_array[id].current_que_index = i;
	amp_rpc_send(amp_wack_array[id].msg_a[i].rec->client, 
		     amp_wack_array[id].msg_a[i].rec->amp_msg->in,
		     amp_wack_array[id].msg_a[i].rec->amp_msg->in_size, 
		     amp_wack_array[id].msg_a[i].rec->amp_msg->inproc,
		     amp_wack_array[id].msg_a[i].msg_cb);
	if (amp_debug)
	  fprintf(stderr,
	    "amp_elapsed_time Msg: %ld\tSeq: %d\tTimestamp: %s\tCTime: %s\n",
		amp_wack_array[id].msg_a[i].msg_cb->message_num,
		amp_wack_array[id].msg_a[i].rec->amp_msg->sequence_num,
		ctime(&amp_wack_array[id].msg_a[i].rec->timestamp),
		ctime(&tp.tv_sec));
	amp_wack_array[id].retransmit = 0;
	if (amp_debug)
	  fprintf(stderr, 
		  "Msg: %ld\tetime: %d\tMax.Trans: %d\tTrans#: %d\n",
		  amp_wack_array[id].msg_a[i].msg_cb->message_num,
		  elapsed_time,
		  amp_wack_array[id].msg_a[i].msg_cb->max_retransmit, 
		  amp_wack_array[id].msg_a[i].rec->transmit_num);
      }
    }
  }
}

/*
 * Sets the starting sequence number to be used
 * for the current process thread.
*/
void
amp_set_seq_num(sequence)
int sequence;
{
  int id;

  if (! amp_get_old_index(&id)) {
    fprintf(stderr,"<amp_set_seq_num> Unknown thread\n");
    return;
  }
  amp_wack_array[id].sequence_num = sequence;
}


/*
 * process ack message. NO_LWP
 */
amp_pack_msg(transp)
     SVCXPRT *transp;
{
  S_AMP_ACK ack_rec;
  if (svc_getargs(transp, xdr_amp_ack, &ack_rec)) {
    return(amp_check_ack_msg(&ack_rec));
  }
  else
    return(0);
}

/*
 * process ack message. LWP
 */
amp_p_ack_msg(xport)
     XDR *xport;
{
  S_AMP_ACK ack_rec;
  if (xdr_amp_ack(xport, &ack_rec)) {
    return(amp_check_ack_msg(&ack_rec));
  }
  else
    return(0);
}


/*
 * Process the acknowledgement message.  Delete the message from
 * the wack array.
*/
amp_check_ack_msg(ack_rec)
S_AMP_ACK *ack_rec;
{
  int i, id, seq;

  if (ack_rec == NULL) {
    fprintf(stderr,"<***amp_check_ack_msg> NULL pointer\n");
    return(0);
  }

  if (! amp_get_old_index(&id)) {
    fprintf(stderr,"<***amp_check_ack_msg> Unknown thread\n");
    return(0);
  }

  for (i=0; i<AMP_MAX_ARRAY_SIZE; i++) {
    if (amp_wack_array[id].msg_a[i].rec != NULL && 
	amp_wack_array[id].msg_a[i].stat == IN_USE &&
	amp_wack_array[id].msg_a[i].msg_cb != NULL) {
      seq = amp_wack_array[id].msg_a[i].rec->amp_msg->sequence_num;
      if ((ack_rec->message_num == 
	   amp_wack_array[id].msg_a[i].msg_cb->message_num) && 
	  (ack_rec->sequence_num == seq)) {
	amp_delete(id, i);
	return(1);
      }
    }
  }
  fprintf(stderr,"amp_check_ack_msg: Seq %d, Msg %d not found in wack array\n",
	  ack_rec->sequence_num, ack_rec->message_num);
  return(0);
}

/*
 * Deletes message info from the wack array.
*/
static
amp_delete(id, i)
int id;
int i;
{
  amp_wack_array[id].msg_a[i].stat = EMPTY;
  if (amp_wack_array[id].msg_a[i].msg_cb != NULL) {
    if (amp_wack_array[id].msg_a[i].msg_cb->data != NULL)
      free (amp_wack_array[id].msg_a[i].msg_cb->data);
    free(amp_wack_array[id].msg_a[i].msg_cb);
  }

  if (amp_wack_array[id].msg_a[i].rec != NULL) {
    if (amp_wack_array[id].msg_a[i].rec->amp_msg != NULL) {
      if (amp_wack_array[id].msg_a[i].rec->amp_msg->in != NULL)
        free (amp_wack_array[id].msg_a[i].rec->amp_msg->in);
      free (amp_wack_array[id].msg_a[i].rec->amp_msg);
    }
    free(amp_wack_array[id].msg_a[i].rec);
  }
  amp_wack_array[id].msg_a[i].msg_cb = NULL;
  amp_wack_array[id].msg_a[i].rec = NULL;
}

/*
 * assigns a new sequence number 
*/
static
amp_get_seq_num(id)
int id;
{
  if (++(amp_wack_array[id].sequence_num) >= AMP_MAX_SEQ_NUM)
    amp_wack_array[id].sequence_num = 0;
  return(amp_wack_array[id].sequence_num);
}


/*
 * return current sequence number 
*/
amp_get_current_seq_num()
{
  int id;

  if (amp_get_old_index(&id))
    return(amp_wack_array[id].sequence_num);
  else
    return(-1);
}

/*
 * adds the message info to the wack array
*/
static
amp_addq(id, rec, msg_cb)
int id;
S_AMP_MSG_REC *rec;
S_AMP_MSG_CB *msg_cb;
{
  int i;

  for (i=0; i<AMP_MAX_ARRAY_SIZE; i++) {
    if (amp_wack_array[id].msg_a[i].stat == EMPTY) {
      amp_wack_array[id].msg_a[i].stat = IN_USE;
      amp_wack_array[id].msg_a[i].rec = rec;
      msg_cb->amp_array_index = i;
      amp_wack_array[id].msg_a[i].msg_cb = msg_cb;
      return(1);
    }
  }
  fprintf(stderr,"***Array is full, can't add message: %d\n",
	  msg_cb->message_num);

  return(0);
}

/*
 * determines whether or not the wack array is empty.
*/
amp_empty_array()
{
  int i, id;

  if (! amp_get_old_index(&id)) {
    fprintf(stderr,"<***amp_empty_array> Unknown thread\n");
    return(0);
  }

  for (i=0; i<AMP_MAX_ARRAY_SIZE; i++)
    if (amp_wack_array[id].msg_a[i].stat == IN_USE)
      return(0);
  return(1);
}

/*
 * prints out all amp thread wack arrays
*/
void
amp_print_all_array()
{
  int i;

  for (i=0; i<AMP_MAX_WACK_ARRAY_SIZE; i++)
    if (amp_wack_array[i].key != -1) {
      fprintf(stderr,"======AMP_WACK_ARRAY[%d]========\n",i);
      amp_print_a(i);
    }
}

/*
 * prints out the current thread wack array
*/
void
amp_print_array()
{
  int id;

  if (amp_get_old_index(&id))
    amp_print_a(id);
  else
    fprintf(stderr,"<***amp_print_array> Unknown thread\n");
}

/*
 * prints out the wack array of thread id
*/
static
amp_print_a(id)
int id;
{
    int i, seq;

    for (i=0; i<AMP_MAX_ARRAY_SIZE; i++) {
      if (amp_wack_array[id].msg_a[i].stat == IN_USE) {
	if (amp_wack_array[id].msg_a[i].rec != NULL) {
	    fprintf(stderr,"====Array: %d==========\n", i);
	    fprintf(stderr,"client address: %x\n", 
		    amp_wack_array[id].msg_a[i].rec->client);
	    fprintf(stderr,"in: %x\tin_proc: %x\n", 
		    amp_wack_array[id].msg_a[i].rec->amp_msg->in, 
		    amp_wack_array[id].msg_a[i].rec->amp_msg->inproc);
	    fprintf(stderr,"timestamp: %ld\n",
		    amp_wack_array[id].msg_a[i].rec->timestamp);
	    seq = amp_wack_array[id].msg_a[i].rec->amp_msg->sequence_num;
	  }
	if (amp_wack_array[id].msg_a[i].msg_cb != NULL) {
	  fprintf(stderr,"message: %d\tseq: %d\n",
		  amp_wack_array[id].msg_a[i].msg_cb->message_num,seq);
	}
      }
    }
}

/*
 * sends out a message
*/
amp_rpc_send(clnt, in, in_size, inproc, msg_cb)
CLIENT *clnt;
char *in;
int in_size;
xdrproc_t inproc;
S_AMP_MSG_CB *msg_cb;
{
    S_AMP_MSG     *msg;
    S_AMP_MSG_REC *rec;
    S_AMP_MSG_CB *cb;
    int clnt_sock = -1, stat;
    struct timeval tp;
    struct timezone tzp;
    int    status = 1, id;
    int    seq, cindex;

    if (clnt == NULL) {
      fprintf(stderr,"***amp_rpc_send: client is NULL\n");
      return(0);
    }

    if (msg_cb == NULL) {
      fprintf(stderr,"***amp_rpc_send: msg_cb is NULL\n");
      return(0);
    }

    if (! amp_get_old_index(&id)) {
      fprintf(stderr,"<amp_rpc_send> Unknown thread\n");
      return(0);
    }

    /*
     * Gets the sequence number for this message
     */
    if (amp_wack_array[id].retransmit) {
      cindex = amp_wack_array[id].current_que_index;
      msg = amp_wack_array[id].msg_a[cindex].rec->amp_msg;
    }
    else if ((msg = (S_AMP_MSG *)malloc(sizeof(S_AMP_MSG))) != NULL) {
      msg->sequence_num = amp_get_seq_num(id);
      if (in_size > 0) {
	msg->in = (char *)malloc(in_size);
	bcopy(in, msg->in, in_size);
      }
      else msg->in = NULL;
      msg->inproc = inproc;
      msg->in_size = in_size;
    }

    if (msg == NULL)
      return(0);

    if (amp_debug)
	fprintf(stderr,"amp[%d] sending message: %d\tSequence: %d\n", 
		id, msg_cb->message_num, msg->sequence_num);

    gettimeofday(&tp, &tzp);

    /*
     * Puts the out going message into the wack array
     */

    if (! amp_wack_array[id].retransmit) {
      rec = (S_AMP_MSG_REC *)malloc(sizeof(S_AMP_MSG_REC));
      rec->client = clnt;
      rec->transmit_num = 0;
      rec->timestamp = tp.tv_sec;
      rec->amp_msg = msg;
      if (amp_debug)
	fprintf(stderr,"amp_rpc_send amp[%d] Msg: %d\tSeq: %d CTime:%s\n", 
	    id, msg_cb->message_num, msg->sequence_num, ctime(&tp.tv_sec));
      cb = (S_AMP_MSG_CB *)malloc(sizeof(S_AMP_MSG_CB));
      cb->message_num = msg_cb->message_num;
      cb->max_wack = msg_cb->max_wack;
      cb->max_retransmit = msg_cb->max_retransmit;
      cb->action = msg_cb->action;
      if (msg_cb->action != NULL && msg_cb->data != NULL 
	  && msg_cb->data_size > 0) {
	cb->data = (char *)malloc(msg_cb->data_size);
	cb->data_size = msg_cb->data_size;
	bcopy(msg_cb->data, cb->data, msg_cb->data_size);
      }
      else cb->data = NULL;
      amp_addq(id, rec, cb);
    }
    else if (amp_wack_array[id].retransmit) {
      cindex = amp_wack_array[id].current_que_index;
      amp_wack_array[id].msg_a[cindex].rec->timestamp = tp.tv_sec;
      amp_wack_array[id].msg_a[cindex].rec->transmit_num++;
    }

    stat = udpcall(clnt, msg_cb->message_num, xdr_amp_msg, 
		   msg, xdr_void, NULL, AMP_UDP_TIMEOUT);

    if (stat != RPC_TIMEDOUT && stat != RPC_SUCCESS) {
      fprintf(stderr,"***udpcall msg %d failed with state %d\n",
	      msg_cb->message_num, stat);
      return(0);
    }
 
    return(1);
}

/*
 * xdr routine for the acknowledgement message
*/
xdr_amp_ack(xdrsp, rec)
     XDR *xdrsp;
     struct amp_ack *rec;
{
  if (!xdr_int(xdrsp, &rec->sequence_num))
    return(0);
  if (!xdr_u_long(xdrsp, &rec->message_num))
    return(0);
  return(1);
}

/*
 * xdr routine for passing amp data structure
*/
xdr_amp_msg(xdrsp, rec)
     XDR *xdrsp;
     struct amp_msg *rec;
{
  if (!xdr_int(xdrsp, &rec->sequence_num))
    return(0);
  return((*rec->inproc)(xdrsp, rec->in));
}

/*
 * Receives an in-coming message
*/

#ifndef NO_LWP

amp_msg_recv(sender, arg, argsize, res, ressize, timeout)
thread_t *sender;
caddr_t *arg;
int *argsize;
caddr_t *res;
int *ressize;
struct timeval *timeout;
{
    int status, len;
    struct amp_msg_t_msg *ptr;
    int ack=1;
    thread_t ssave;

    ssave = *sender;
    while (ack) {
      *sender = ssave;
      status = msg_recv(sender, arg, argsize, res, ressize, timeout);
      
      if (status == 0) {
	ptr = (struct amp_msg_t_msg *)*arg;
	if (ack = amp_p_msg(ptr)) {
	  if (amp_debug>1)
	     fprintf(stderr,"<amp_msg_recv> Send reply for ACK: %d\n",	
		*(u_long *)ptr);	
	  msg_reply(*sender);
	}
      }
      else if ((status == -1) && (lwp_geterr() == LE_TIMEOUT)) {
	ack = 0;
	if (amp_debug>1)
	  fprintf(stderr,"<amp_msg_recv> TIMEOUT.  Check elapsed time\n");
	amp_elapsed_time();
      }
    }
    return(status);
}

/*
 * Receives an in-coming message from any sender
*/
amp_msg_recv_all(sender, arg, argsize, res, ressize, timeout)
thread_t *sender;
caddr_t *arg;
int *argsize;
caddr_t *res;
int *ressize;
struct timeval *timeout;
{
  *sender = THREADNULL;
  
  return(amp_msg_recv(sender, arg, argsize, res, ressize, timeout));
}
#endif

/*
 * process the in-coming message.
*/
static int
amp_p_msg(ptr)
     struct amp_msg_t_msg *ptr;
{
  u_long msg;

  msg = *(u_long *)ptr;
    
  if (is_ack_msg(msg)) {
    S_AMP_ACK ack_rec;
    SVCXPRT   *transp;
    S_AMP_MSG_T_MSG *rec;
    XDR       *xport;

    rec = (S_AMP_MSG_T_MSG *)ptr;
    xport = (XDR *)rec->xport;
    if (xdr_amp_ack(xport, &ack_rec))
      amp_check_ack_msg(&ack_rec);
    return(1);  /* ack msg */
  }
  else {
    amp_elapsed_time();
    return(0);  /* not an ack */
  }
}


static
is_ack_msg(msg)
u_long msg;
{
  if (msg >= 30000)
    return(0);
  if (msg % 100 == 0)
    return(1);
  else
    return(0);
}

