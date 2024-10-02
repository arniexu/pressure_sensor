#ifndef FMB_TCP
#define FMB_TCP
#include "f2f_modbus/f2f_modbus.h"

#define MAX_TCP_FD  20
#define TCP_BUF_LEN		65535

struct modbus_tcp_pack
{
	unsigned short transid	:16;
	unsigned short protoid	:16;
	unsigned short len		:16;
	unsigned short addr		:8;
	unsigned char pdu[2048];
};
struct fmb_tcp
{
	bool_t running;
    bool_t binded;
	int fd;
	
	pthread_t thread;
	int 				fds[MAX_TCP_FD];
	char 				buf[MAX_TCP_FD][TCP_BUF_LEN];
	size_t				data_len[MAX_TCP_FD];
	struct sockaddr_in 	addrs[MAX_TCP_FD];
	struct timeval 		last_active[MAX_TCP_FD];
	unsigned short 		last_transid[MAX_TCP_FD];
	int (*cb)(void *tcp, fmb_transport_type_e trans, int fd, void *data, size_t len, bool_t is_broadcast, void *user_data);
	void *user_data;
};
struct fmb_tcp_data
{
	uint16_t total;
	uint16_t len;
	in_port_t remote_port;
	in_addr_t remote_ip;
	int fd;
	pthread_mutex_t *mutex;
	void *data;
};

#ifdef __cplusplus
extern "C" {
#endif

	fmb_tcp_t *fmb_tcp_init();
	int fmb_tcp_uninit(fmb_tcp_t *tcp);

	int fmb_tcp_bind(fmb_tcp_t *tcp, in_addr_t local_ip, in_port_t local_port);
	int fmb_tcp_send(fmb_tcp_t *tcp, int fd, void *data, size_t len);
	int fmb_tcp_set_recved_cb(fmb_tcp_t *tcp, int (*cb)(void *tcp, fmb_transport_type_e trans, int fd, void *data, size_t len, bool_t is_broadcast, void *user_data), void *user_data);

#ifdef __cplusplus
}
#endif
#endif