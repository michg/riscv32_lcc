/*
 * sys/socket.h
 */

#ifndef	__SYS_SOCKET_H__
#define	__SYS_SOCKET_H__

/*-----------------*/
/*  Socket types   */
/*-----------------*/
#define SOCK_STREAM     1      /* stream socket */
#define SOCK_DGRAM      2      /* datagram socket */
#define SOCK_RAW        3      /* raw-protocol interface */
#define SOCK_RDM        4      /* reliably-delivered message */
#define SOCK_SEQPACKET  5      /* sequenced packet stream */

/*-----------------*/
/* Protocols       */
/*-----------------*/
/*
 * Address families.
 */
#define AF_UNSPEC       0      /* unspecified */
#define AF_UNIX         1      /* local to host (pipes, portals) */
#define AF_INET         2      /* internetwork: UDP, TCP, etc. */

/*-----------------*/
/* Send/Revc Flags */
/*-----------------*/
#define MSG_OOB         0x1    /* process out-of-band data */
#define MSG_PEEK        0x2    /* peek at incoming message */
#define MSG_DONTROUTE   0x4    /* send without using routing tables */


/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 */
#define	IN_CLASSA(i)		(((long)(i) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		0x00ffffff
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)		(((long)(i) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		0x0000ffff
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)		(((long)(i) & 0xe0000000) == 0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		0x000000ff

#define	IN_CLASSD(i)		(((long)(i) & 0xf0000000) == 0xe0000000)
#define	IN_CLASSD_NET		0xf0000000	/* These ones aren't really */
#define	IN_CLASSD_NSHIFT	28		/* net and host fields, but */
#define	IN_CLASSD_HOST		0x0fffffff	/* routing needn't know.    */
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define	IN_EXPERIMENTAL(i)	(((long)(i) & 0xe0000000) == 0xe0000000)
#define	IN_BADCLASS(i)		(((long)(i) & 0xf0000000) == 0xf0000000)

#define	INADDR_ANY		(unsigned int)0x00000000
#define	INADDR_LOOPBACK		(unsigned int)0x7F000001
#define	INADDR_BROADCAST	(unsigned int)0xffffffff	/* must be masked */

#define	INADDR_UNSPEC_GROUP	(unsigned int)0xe0000000	/* 224.0.0.0   */
#define	INADDR_ALLHOSTS_GROUP	(unsigned int)0xe0000001	/* 224.0.0.1   */
#define	INADDR_ALLRTRS_GROUP	(unsigned int)0xe0000002	/* 224.0.0.2   */
#define	INADDR_MAX_LOCAL_GROUP	(unsigned int)0xe00000ff	/* 224.0.0.255 */

#define	IN_LOOPBACKNET		127			/* official! */



/*
 * Macro-rename the functions here,
 * to avoid clashes with other tcp-ip
 * libraries (e.g. pSOS networking
 * component pNA):
 */

#define socket       _rpc_socket
#define accept       _rpc_accept
#define bind         _rpc_bind
#define connect      _rpc_connect
#define recv         _rpc_recv
#define send         _rpc_send
#define closesocket  _rpc_closesocket
#define inet_addr    _rpc_inet_addr
#define listen       _rpc_listen


#ifdef __LITTLE_ENDIAN__
	#define htonl(addr)     ( (((unsigned long)(addr) & 0x000000FF)<<24) | \
				  (((unsigned long)(addr) & 0x0000FF00)<<8)  | \
				  (((unsigned long)(addr) & 0x00FF0000)>>8)  | \
				  (((unsigned long)(addr) & 0xFF000000)>>24))
	
	#define ntohl(addr)     htonl(addr)
	
	#define htons(addr)     ( (((unsigned short)(addr) & 0x000000FF)<<8)  | \
				  (((unsigned short)(addr) & 0x0000FF00)>>8))
	
	#define ntohs(addr)     htons(addr)
#else
	#define htonl(a)       ((unsigned long)(a))
	#define ntohl(a)       ((unsigned long)(a))
	#define htons(a)       ((unsigned short)(a))
	#define ntohs(a)       ((unsigned short)(a))
#endif




/*------------------------*/
/* Generic Socket address */
/*------------------------*/
struct sockaddr
  {
  unsigned short     sa_family;
  char               sa_data[14];
  };

/*---------------------------*/
/* Berkeley Internet address */
/*---------------------------*/
struct in_addr
    {
    unsigned long    s_addr;      
    };

/*-------------------------*/
/* Internet style address  */
/*-------------------------*/
struct sockaddr_in
    {
    short            sin_family;           
    unsigned short   sin_port;    
    struct in_addr   sin_addr;    
    char             sin_zero[8];           
    };


int             socket      (int af, int type, int protocol);
int             accept      (int s, struct sockaddr *addr, int  *addrlen);
int             bind        (int s, struct sockaddr *addr, int   addrlen );
int             connect     (int s, struct sockaddr *addr, int   addrlen );
int             recv        (int s, char  * buf, int len, int flags);
int             send        (int s, char  * buf, int len, int flags);
int             closesocket (int s);
unsigned long   inet_addr   (char * cp);
int             listen      (int s, int backlog);

#endif	/* __SYS_SOCKET_H__ */

/* end of sys/types.h */
