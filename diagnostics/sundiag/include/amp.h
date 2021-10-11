/*	@(#)ATS amp.h 1.1 92/07/30 Copyright Sun Microsystems Inc.	*/

typedef struct amp_ack {
  int sequence_num;
  u_long  message_num;
} S_AMP_ACK;


typedef struct amp_msg_cb {
    u_long message_num;
    int max_wack;       /* maximum ack waiting time */
    int max_retransmit; /* maximum retransmission */
    char *data;         /* parameter to function action */
    int  data_size;     /* size of data */
    void (*action)();  /* execuated after max_retransmit */
    int amp_array_index;  /* index to S_AMP_MSG_ARRAY. Filled in by amp */
} S_AMP_MSG_CB;


typedef struct amp_msg_t_msg {
  u_long  message_num;
  XDR    *xport;
} S_AMP_MSG_T_MSG;


typedef struct amp_msg {
  int sequence_num;
  char *in;
  xdrproc_t inproc;
  int in_size;
}S_AMP_MSG;

typedef struct amp_msg_rec {
    CLIENT *client;
    u_long timestamp;
    S_AMP_MSG *amp_msg;
    int transmit_num;
} S_AMP_MSG_REC;

enum msg_array_stat {
    EMPTY=0,
    IN_USE=1
    };

typedef struct amp_msg_array {
    enum msg_array_stat stat;
    S_AMP_MSG_REC *rec;
    S_AMP_MSG_CB *msg_cb;
} S_AMP_MSG_ARRAY;

extern int amp_debug;
extern int amp_initialize();
extern int amp_exit();
extern int amp_rpc_send();
extern int amp_send_ack();
extern int amp_transp_send_ack();
extern void amp_elapsed_time();
extern int amp_check_ack_msg();
extern int amp_p_ack_msg();
extern int amp_empty_array();
extern int amp_get_current_seq_num();
extern void amp_print_array();
extern void amp_print_all_array();
extern int amp_flush_msg_array();
extern int amp_msg_recv();
extern int amp_msg_recv_all();
extern void amp_set_seq_num();

extern xdr_amp_ack();
extern xdr_amp_msg();
