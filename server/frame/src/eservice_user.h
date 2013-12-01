/************ eservice user api *************/

#ifndef	_ESERVICE_albert_
#define	_ESERVICE_albert_	1

#include	"albert_log.h"
#include	"common_albert.h"
#include	"dbllist.h"

struct eservice_unit_t;
struct eservice_manager_t;
struct eservice_buf_t;

struct eservice_timer_t
{
	struct event *timer_event;
};

/* user callback return value */
typedef enum {
	/* for eservice_cb_data_arrive */
	eservice_cb_data_arrive_fatal = 50,			/* close immediately */
	eservice_cb_data_arrive_not_enough,			/* continue read */
	eservice_cb_data_arrive_finish,				/* close softly */
	eservice_cb_data_arrive_close,				/* close after write */
	eservice_cb_data_arrive_ok,				/* to write */

	/* for other callback */
	eservice_cb_success = 0,					/* success */
	eservice_cb_failed = -1,						/* failed */
} eservice_callback_return_val;

enum {
	eservice_user_errcode_invalid = -100,
	eservice_user_errcode_write = -101,
	eservice_user_errcode_mm = -119,
};

typedef	int (*eservice_buf_process_callback)(eservice_unit_t *eup, eservice_buf_t *ebp, void *arg);
typedef eservice_callback_return_val (*eservice_user_process_callback)(struct eservice_manager_t *egr, eservice_unit_t *eup, void *earg, void *garg);

#define	ESERVICE_DESTROY_EBP(ebp)	do {if ((ebp)) {eservice_buf_free((ebp)); } (ebp) =NULL;} while (0)

#ifdef	__cplusplus
extern "C" {
#endif

/***************************** ebuf api start ********************************/

/**
  A cleanup function for a piece of memory added to an ebuf by reference.

   @see eservice_buf_add_reference()
 */
typedef void (*eservice_buf_ref_cleanup_cb)(const void *data, size_t datalen, void *extra);



/**
  Allocate storage for a new ebuf.

  @param init_size is the init linearizes space
  @return a pointer to a newly allocated ebuf struct, or NULL if an error occurred
 */
extern struct eservice_buf_t *eservice_buf_new(size_t init_size);



/**
  Deallocate storage for an ebuf.

  @param ebp pointer to the ebuf to be freed
 */
extern void eservice_buf_free(struct eservice_buf_t *ebp);



/**
  Returns the total number of bytes stored in the ebuf.

  @param ebp pointer to the ebuf
  @return the number of bytes stored in the ebuf
*/
extern ssize_t eservice_buf_datalen(struct eservice_buf_t *ebp);



/**
  Returns a pointer to the data pos of the ebuf.

  @param ebp pointer to the ebuf
  @param from_head_len is the offset of data
  @assure_linearizes_len if the linearizes len from from_head_len, or -1 if all rear data
  @return the pointer
*/
extern uint8_t *eservice_buf_pos(struct eservice_buf_t *ebp, uint32_t from_head_len, ssize_t assure_linearizes_len);



/**
  Returns a pointer to the beginning of the ebuf.

  @param ebp pointer to the ebuf
  @return the beginning data pointer
*/
extern uint8_t *eservice_buf_start(struct eservice_buf_t *ebp);

extern uint8_t *eservice_buf_start_ex(struct eservice_buf_t *ebp);


/**
  Prepends data to the beginning of the ebuf

  @param ebp the ebuf to which to prepend data
  @param data a pointer to the memory to prepend
  @param size the number of bytes to prepend
  @return 0 if successful, or -1 otherwise
*/
extern int eservice_buf_prepend(struct eservice_buf_t *ebp, const void *data, size_t size);



/**
  Set control callback to the ebuf.

  @param ebp the ebuf to which to prepend data
  @param retry_num how many times retry before give up when sending data
  @param failed_cb when sending failed, note user in this callback
  @param arg optinal sending failed callback argument
  @return 0 if successful, or -1 otherwise
*/
extern int eservice_buf_set_control(struct eservice_buf_t *ebp, uint32_t retry_num, eservice_buf_process_callback failed_cb, void *arg);



/**
   Reserves space of linearizes len.

   @param ebp the ebuf in which to reserve space.
   @param want_len how much space to make available and linearizesd
   @param space_posp output pointer to the memory space we got
   @param space_lenp output pointer to the space len we got
   @return 0 if successful, or -1 otherwise
   @see eservice_buf_commit_space()
*/
extern int eservice_buf_get_space(struct eservice_buf_t *ebp, size_t want_len, uint8_t **space_posp, size_t *space_lenp);



/**
   Commits previously reserved space.

   Commits some of the space previously reserved with
   eservice_buf_get_space().  It then becomes available for reading.

   This function may return an error if the pointer in the extents do
   not match those returned from eservice_buf_get_space, or if data
   has been added to the buffer since the space was reserved.

   If you want to commit less data than you got reserved space for,
   just give a smaller len value.  Note that you may have received more space than you
   requested if it was available!

   @param ebp the ebuf in which to reserve space.
   @param vec one or two extents returned by evbuffer_reserve_space.
   @param n_vecs the number of extents.
   @return 0 on success, -1 on error
   @see eservice_buf_get_space()
*/
extern int eservice_buf_commit_space(struct eservice_buf_t *ebp, uint8_t *space_pos, size_t space_len);



/**
   Search for a string within an ebuf from beginning, when found, make all data linearizesd after the first occurrence of the string (include the string it self).

   @param ebp the ebuf to be searched
   @param what the string to be searched for
   @param len the length of the search string
   @return a pointer to the first occurrence of the string in the buffer, NULL if the string was not found.
 */
extern uint8_t *eservice_buf_find(struct eservice_buf_t *ebp, const unsigned char * what, size_t len);



/**
   Make the data in ebuf contiguous.

   @param ebp the ebuf to be linearized
   @param len the number of bytes to be linearized, or -1 if all
   @return 0 on success, -1 on failure
 */
extern int eservice_buf_make_linearizes(struct eservice_buf_t *ebp, ssize_t len);



/**
  Remove a specified number of bytes data from the beginning of an ebuf.

  @param ebp the ebuf to be drained
  @param len the number of bytes to drain from the beginning of the buffer
  @return 0 on success, -1 on failure.
 */
extern int eservice_buf_drain(struct eservice_buf_t *ebp, size_t len);



/**
  Append data to the end of an ebuf.

  @param ebp the ebuf to append data
  @param data the pointer to the beginning of the data to append
  @param datlen the number of bytes to append to the ebuf
  @return 0 on success, -1 on failure.
 */
extern int eservice_buf_add(struct eservice_buf_t *ebp, const void * data, size_t datlen);



/**
  Append data to the end of an ebuf without copying.

  The memory needs to remain valid until all the added data has been
  read.  This function keeps just a reference to the memory without
  actually incurring the overhead of a copy.

  @param ebp the ebuf to append data
  @param data the pointer to the beginning of the data to append
  @param datlen the number of bytes to append to the ebuf
  @param cleanupfn callback to be invoked when the memory is no longer referenced by this ebuf.
  @param cleanupfn_arg optional argument to the cleanup callback
  @return 0 on success, -1 on failure.
 */
extern int eservice_buf_clone_data(struct eservice_buf_t *dst, struct eservice_buf_t *src);
extern int eservice_buf_add_reference(struct eservice_buf_t *ebp, const void *data, size_t datlen, eservice_buf_ref_cleanup_cb cleanupfn, void *cleanupfn_arg);



/**
  Move data from a ebuf to another ebuf, copying as little as possible.

  @param dst the destination ebuf to store the result into
  @param src the ebuf to be read from
  @param datalen the number of bytes to move, or -1 if all
  @return 0 on success, -1 on failure.
 */
extern int eservice_buf_move_data(struct eservice_buf_t *dst, struct eservice_buf_t *src, ssize_t datalen);



/**
  Get the ebuf pointer from the double link list in an ebuf.

  @param listp the double link list of one ebuf
  @return the ebuf pointer.
 */
extern struct eservice_buf_t *eservice_buf_list2buf(struct dbl_list_head *listp);



/**
  Get the double link list pointer from an ebuf.

  @param ebp the ebuf
  @return the double link list pointer.
 */
extern struct dbl_list_head *eservice_buf_buf2list(struct eservice_buf_t *ebp);

/***************************** ebuf api end ********************************/







/************************* eservice user api start ****************************/

/**
  Pop an ebuf from the connection's read buffer with specisfic length.

  @param eup the connection
  @param pop_size the length of data pop
  @param result_pkgp the address of an ebuf pointer stores the result poped ebuf, NULL if data are not enough
  @return 0 on success, -1 on failure.
 */
extern int eservice_user_pop_inbuf(struct eservice_unit_t *eup, ssize_t pop_size, struct eservice_buf_t **result_pkgp);



/**
  Push an ebuf to the connection's write buffer.

  When this function called, whether success or failure, the ebuf to push is taken over by the frame, 
  the user should not use it any longer.

  @param eup the connection
  @param ebp the ebuf to push
  @return 0 on success, -1 on failure.
 */
extern int eservice_user_push_outbuf(struct eservice_unit_t *eup, struct eservice_buf_t *ebp);




extern uint32_t eservice_user_get_cdnproxy_client_ip(eservice_unit_t *eup);


extern uint32_t eservice_user_set_cdnproxy_client_ip(eservice_unit_t *eup, uint32_t clientip);

/**
  Set the the left number of data needed in a connection.

  @param eup the connection
  @param want_len specific the number of data to read more, or -1 if as many as we can
 */
extern void eservice_user_next_need(struct eservice_unit_t *eup, ssize_t want_len);



/**
  Returns a pointer to the data pos of the connection's read buffer.

  @param eup pointer to the connection
  @param from_head_len is the offset of data
  @assure_linearizes_len if the linearizes len from from_head_len, or -1 if all rear data
  @return the pointer.
*/
extern uint8_t *eservice_user_pos(struct eservice_unit_t *eup, uint32_t from_head_len, ssize_t assure_linearizes_len);

extern uint8_t *eservice_user_pos_ex(struct eservice_unit_t *eup, uint32_t from_head_len, ssize_t assure_linearizes_len);

/**
  Returns a pointer to the beginning of the connection's read buffer.

  @param eup pointer to the connection
  @return the beginning data pointer
*/
extern uint8_t *eservice_user_start(struct eservice_unit_t *eup);



/**
   Search for a string within an connection's read buffer from beginning, when found, make all data linearizesd after the first occurrence of the string (include the string it self).

   @param eup the connection
   @param what the string to be searched for
   @param len the length of the search string
   @return a pointer to the first occurrence of the string in the buffer, NULL if the string was not found.
 */
extern uint8_t *eservice_user_find(eservice_unit_t *eup, const unsigned char * what, size_t len);



/**
  Close the connection when the write buffer is empty.

  @param eup the connection
  @return 0 on success, or -1 on failure
 */
extern int eservice_user_set_close(eservice_unit_t *eup);




extern int eservice_user_drain(eservice_unit_t *eup, size_t len);
extern ssize_t eservice_user_datalen(struct eservice_unit_t *eup);
extern int eservice_user_conn_idle_seconds(eservice_unit_t *eup);
extern int eservice_user_conn_use_seconds(eservice_unit_t *eup);
extern int eservice_user_conn_is_connecting(eservice_unit_t *eup);
extern int eservice_user_conn_set_user_data(eservice_unit_t *eup, void *user_data);
extern void *eservice_user_conn_get_user_data(eservice_unit_t *eup);

extern struct eservice_unit_t *eservice_user_register_tcp_conn(struct eservice_unit_t *already_eup, const char *ip, const char *port, int reconn_ms, struct timeval *to);
extern struct eservice_unit_t *eservice_user_register_tcp_imme(struct eservice_unit_t *already_eup, const char *ip, const char *port, int reconn_ms, struct timeval *to);
extern struct sockaddr_in* eservice_user_get_register_tcp_peer(struct eservice_unit_t *eup);
extern int eservice_user_set_tcp_rw_buffer_size(struct eservice_unit_t *eup, uint32_t read_buffer_size, uint32_t write_buffer_size);
extern int eservice_user_set_tcp_conn_timeout_cb(struct eservice_unit_t *eup, eservice_user_process_callback conn_timeout_cb, void *conn_timeout_arg, uint32_t to_ms);
extern int eservice_user_set_tcp_conn_data_arrive_cb(struct eservice_unit_t *eup, eservice_user_process_callback data_arrive_cb, void *data_arrive_arg);
extern int eservice_user_set_tcp_conn_terminate_cb(struct eservice_unit_t *eup, eservice_user_process_callback terminate_cb, void *terminate_arg);
extern int eservice_user_destroy_tcp_conn(struct eservice_unit_t *eup);

extern const char *eservice_user_get_peer_addr_str(struct eservice_unit_t *eup);
extern network32_t eservice_user_get_peer_addr_network(struct eservice_unit_t *eup);
extern host16_t eservice_user_get_peer_addr_port(struct eservice_unit_t *eup);
extern const char *eservice_user_get_local_addr_str(struct eservice_unit_t *eup);
extern network32_t eservice_user_get_local_addr_network(struct eservice_unit_t *eup);
extern host16_t eservice_user_get_local_addr_port(struct eservice_unit_t *eup);
extern host16_t eservice_user_get_related_port(struct eservice_unit_t *eup);

extern int eservice_user_timer(struct eservice_manager_t *egr, struct eservice_timer_t *timerp, struct timeval *to, void (*eservice_user_timer_cb)(int, short, void *), void *arg);
extern int eservice_user_eup_is_invalid(struct eservice_unit_t *eup);
extern int eservice_user_set_tcp_nodelay(struct eservice_unit_t *eup, int is_nodelay);

extern int eservice_user_eup_to_index(struct eservice_unit_t *eup);
extern eservice_unit_t *eservice_user_eup_from_index(int index);

/************************* eservice user api end ****************************/

#ifdef	__cplusplus
}
#endif

#endif

