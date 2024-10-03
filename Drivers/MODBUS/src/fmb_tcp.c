#include "fmb_tcp.h"

// static void *__fmb_tcp_data_new(uint32_t data_size, unsigned short header_size, unsigned int *in_use)
// {
// 	fmb_tcp_data_t *d;

// 	d = fbs_new(fmb_tcp_data_t, 1);

// 	d->len = 0;
// 	d->total = data_size;
// 	if(d->total > 0)
// 		d->data = fbs_new(unsigned char, d->total);
// 	else
// 		d->data = NULL;
// 	d->mutex = fbs_mutex_create();

// 	return (void*)d;
// }
#if 0
fmb_tcp_t *fmb_tcp_init()
{
    int val;
    struct linger linger;
    struct timeval timeout;
    fmb_tcp_t *tcp;
    
	tcp = fbs_new(fmb_tcp_t, 1);
	tcp->binded = FALSE;
	tcp->running = TRUE;
	// tcp->recv_buf = fbs_cycle_new();
    // fbs_cycle_add(tcp->recv_buf, &__fmb_tcp_data_new, atoi(fbs_cfg_get(":modbus_buf_len", CFG_PROTO)), 0, atoi(fbs_cfg_get(":modbus_buf_size", CFG_PROTO)));

	tcp->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp->fd <= 0)
	{
		fbs_err(MOD_MODBUS, "failed to create tcp sock", NULL);
		fbs_free(tcp);
		return NULL;
	}

	//linger
	linger.l_linger = 0;
	linger.l_onoff = 1;
	if(setsockopt(tcp->fd, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger)) < 0)
    {
        fbs_err(MOD_MODBUS, "failed to setsockopt SO_LINGER", NULL);
        fbs_free(tcp);
        return NULL;
    }

	//no delay解决粘包问题
	val = 1;
    if(setsockopt(tcp->fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&val, sizeof(int)) < 0)
    {
        fbs_err(MOD_MODBUS, "failed to setsockopt TCP_NODELAY", NULL);
        fbs_free(tcp);
        return NULL;
    }

    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
	if(setsockopt(tcp->fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(struct timeval)) < 0)
	{
		fbs_err(MOD_MODBUS, "failed to setsockopt SO_SNDTIMEO", NULL);
		fbs_free(tcp);
		return NULL;
	}

    return tcp;
}
// static void __fmb_tcp_data_destroy(void *arg)
// {
// 	fmb_tcp_data_t *d;

// 	if(arg == NULL)
// 		return;
// 	d = (fmb_tcp_data_t*)arg;

// 	fbs_mutex_destroy(d->mutex);
// 	if(d->total && d->data)
// 		fbs_free(d->data);
// 	fbs_free(d);
// }
int fmb_tcp_uninit(fmb_tcp_t *tcp)
{
	if(tcp == NULL)
		return FBS_NULL_PARAM;

	tcp->running = FALSE;
	if(tcp->thread)
		fbs_thread_join(tcp->thread, NULL);

	// fbs_cycle_clear(tcp->recv_buf, __fmb_tcp_data_destroy);
	// fbs_cycle_destroy(tcp->recv_buf);

	shutdown(tcp->fd, SHUT_RDWR);
	close(tcp->fd);
	fbs_free(tcp);
	return FBS_SUCC;
}
static int __fmb_tcp_peer_close_or_timeout(int *maxfd, fd_set *set_back, int pos, fmb_tcp_t *tcp)//, fmb_tcp_data_t *in)
{
	int j;
	*maxfd = 0;
	for(j = 0; j < MAX_TCP_FD; j++)
		*maxfd = fbs_max(*maxfd, tcp->fds[j]);

	FD_CLR(tcp->fds[pos], set_back);
	close(tcp->fds[pos]);

	// ((char*)in->data)[0] = '\0';
	// in->len = 0;
	// in->fd = tcp->fds[pos];
	// in->remote_ip = tcp->addrs[pos].sin_addr.s_addr;
	// in->remote_port = tcp->addrs[pos].sin_port;

	memset(&tcp->addrs[pos], 0, sizeof(struct sockaddr_in));
	memset(&tcp->last_active[pos], 0, sizeof(struct timeval));
	tcp->fds[pos] = -1;

	return FBS_SUCC;
}
static void *__fmb_tcp_server_thread(void *arg)
{
    modbus_tcp_pack_t *pack;
    fd_set set, set_back;
    int i, nfound, maxfd, connfd, timeout;
    const char *p;
    ssize_t read_len = 0;
    struct timeval tv, now;
    struct sockaddr_in addr;
    socklen_t len;
    fmb_tcp_t *tcp = (fmb_tcp_t*)arg;

	p = fbs_cfg_get("modbus:timeout", CFG_PROTO);
	if(p)
		timeout = atoi(p) * 1000;
	else
		timeout = 0;
    memset(&tcp->fds, -1, sizeof(tcp->fds));
	tcp->fds[0] = tcp->fd;
    FD_ZERO(&set_back);
    FD_SET(tcp->fd, &set_back);
    maxfd = tcp->fd;

    while(tcp->running)
    {
    	fbs_thread_call(0);
        if(tcp->fd <= 0)
            break;
        set = set_back;
        tv.tv_sec = 0;
        tv.tv_usec = 1000 * 100;

        if((nfound = select(maxfd + 1, &set, (fd_set *)0, (fd_set *)0, &tv)) < 0)
        {
            fbs_err(MOD_MODBUS, "failed to select errno:%d", errno);
            break;
        }
        else if(nfound == 0)
        {
			fbs_thread_call(1);
        	if(timeout == 0)
                continue;
            gettimeofday(&now, NULL);
            for(i = 1; i < MAX_TCP_FD; i++)
            {
                if(tcp->fds[i] <= 0 || tcp->last_active[i].tv_sec == 0 || tcp->last_active[i].tv_usec == 0 
                    || fbs_timeval_diff_in_ms(now, tcp->last_active[i]) < timeout)
                    continue;
                fbs_msg(MOD_MODBUS, "peer timeout fd:%d ip:%s:%d", tcp->fds[i], inet_ntoa(tcp->addrs[i].sin_addr), ntohs(tcp->addrs[i].sin_port));
                __fmb_tcp_peer_close_or_timeout(&maxfd, &set_back, i, tcp);
            }
            continue;
        }

		fbs_thread_call(1);
        //检测监听套接字是否可读
        if(FD_ISSET(tcp->fd, &set))
        {
            //可读，证明有新客户端连接服务器
            len = sizeof(addr);
            connfd = accept(tcp->fd, (struct sockaddr*)&addr, &len);
            if(connfd < 0)
            {
                fbs_err(MOD_MODBUS, "fail to accept new connection", NULL);
                continue;
            }

            //将新连接套接字加入fd_all及fd_read
            for(i = 0; i < MAX_TCP_FD; i++)
            {
                if(tcp->fds[i] != -1)
                    continue;

				tcp->fds[i] = connfd;
				tcp->addrs[i].sin_port = addr.sin_port;
				tcp->addrs[i].sin_addr.s_addr = addr.sin_addr.s_addr;
				tcp->addrs[i].sin_family = addr.sin_family;
				gettimeofday(&tcp->last_active[i], NULL);
				fbs_msg(MOD_MODBUS, "new conn from fd:%d ip:%s:%d", tcp->fds[i], inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                break;
            }

            FD_SET(connfd, &set_back);

            //更新maxfd
            if(maxfd < connfd)
                maxfd = connfd;
            continue;
        }
        for(i = 1; i < MAX_TCP_FD; i++)
        {
            if (tcp->fds[i] == -1 || !FD_ISSET(tcp->fds[i], &set))
            	continue;
			nfound--;
			read_len = recv(tcp->fds[i], tcp->buf[i] + tcp->data_len[i], TCP_BUF_LEN - tcp->data_len[i], 0);
			if(read_len > 0)
			{
                // int j;
                // printf("recv:%dbytes", read_len);
                // for(j = 0; j < read_len; j++)
                // {
                //     printf(" 0x%02x", tcp->buf[i][j]);
                // }
                // printf("\n");

				gettimeofday(&tcp->last_active[i], NULL);

                pack = (modbus_tcp_pack_t*)tcp->buf[i];
                //不是发给我的，直接把数据清掉
                if(pack->addr != 0x0 && pack->addr != atoi(fbs_cfg_get("modbus:slaveid", CFG_PROTO)))
                {
                    tcp->data_len[i] = 0;
                    continue;
                }
                tcp->last_transid[i] = ntohs(pack->transid);
                tcp->data_len[i] += read_len;
                pack->len = ntohs(pack->len);
                //数据还没有收全
                if(tcp->data_len[i] < (pack->len + 6))
                    continue;
                // printf("transid:%u len:%u addr:%u\n", tcp->last_transid[i], pack->len, pack->addr);
                

                if(tcp->cb)
                    tcp->cb(tcp, FMB_TRANSPORT_TCP, tcp->fds[i], pack->pdu, pack->len - 1, pack->addr == 0, tcp->user_data);
                tcp->data_len[i] = 0;
			}
			else
			{
				fbs_msg(MOD_MODBUS, "peer shutdown fd:%d ip:%s:%d", tcp->fds[i], inet_ntoa(tcp->addrs[i].sin_addr), ntohs(tcp->addrs[i].sin_port));
				__fmb_tcp_peer_close_or_timeout(&maxfd, &set_back, i, tcp);//, in);
			}
        }
    }
    return NULL;
}
int fmb_tcp_bind(fmb_tcp_t *tcp, in_addr_t local_ip, in_port_t local_port)
{
    int val;
    struct sockaddr_in addr;

    if(tcp == NULL)
        return FBS_NULL_PARAM;

    if(tcp->binded)
        return FBS_SUCC;

    val = 1;
    if(setsockopt(tcp->fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(int)) < 0)
    {
        fbs_err(MOD_MODBUS, "failed to setsockopt SO_REUSEADDR", NULL);
        fbs_free(tcp);
        return FBS_SYS_CALL_FAILED;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = local_port;
    addr.sin_addr.s_addr = local_ip;
    if(bind(tcp->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fbs_err(MOD_MODBUS, "failed to bind to %s:%d", inet_ntoa(addr.sin_addr), ntohs(local_port));
        return FBS_SYS_CALL_FAILED;
    }
    if(listen(tcp->fd, MAX_TCP_FD) < 0)
    {
        fbs_err(MOD_MODBUS, "failed to listen", NULL);
        return FBS_SYS_CALL_FAILED;
    }
    tcp->binded = TRUE;
    if(tcp->thread == 0)
        tcp->thread = fbs_thread_create(__fmb_tcp_server_thread, tcp, "fmb_tcp_server");
	fbs_msg(MOD_MODBUS, "binded to %s:%d ok", inet_ntoa(addr.sin_addr), ntohs(local_port));
    return FBS_SUCC;
}
int fmb_tcp_send(fmb_tcp_t *tcp, int fd, void *data, size_t len)
{
    int i;
    char *buf;
    modbus_tcp_pack_t *pack;

    for(i = 1; i < MAX_TCP_FD; i++)
    {
        if(fd != tcp->fds[i])
            continue;
        break;
    }
    if(i >= MAX_TCP_FD)
        return FBS_NOTFOUND;

    buf = fbs_new(char, len + 7);
    pack = (modbus_tcp_pack_t*)buf;
    pack->transid = htons(tcp->last_transid[i]);
    pack->protoid = 0;
    pack->addr = atoi(fbs_cfg_get("modbus:slaveid", CFG_PROTO));
    pack->len = htons(len + 1);
    memcpy(pack->pdu, data, len);

    // int j;
    // printf("rsp:%dbytes", len + 7);
    // for(j = 0; j < len + 7; j++)
    // {
    //     printf(" 0x%02x", buf[j]);
    // }
    // printf("\n");
    i = send(fd, buf, len + 7, 0);
    fbs_free(buf);
    return i;
}
int fmb_tcp_set_recved_cb(fmb_tcp_t *tcp, int (*cb)(void *tcp, fmb_transport_type_e trans, int fd, void *data, size_t len, bool_t is_broadcast, void *user_data), void *user_data)
{
    if(tcp == NULL)
        return FBS_NULL_PARAM;
    tcp->cb = cb;
    tcp->user_data = user_data;
    return FBS_SUCC;
}
#endif
