#include "fmb_core.h"
#include "main.h"

void *__fmb_core_private_proto_thread(void *arg)
{
    fmb_pdu_rsp_t pdu_rsp;
    int wait_total, max_wait, rand;
    fmb_manage_t *m;
    fmb_core_t *c = (fmb_core_t*)arg;

    while(c->running)
    {
        while(!c->pp_send && c->running)
        {
            HAL_Delay(10);
        }
        if(!c->running)
            break;

        wait_total = 0;
        rand = 0;
        max_wait = atoi(fbs_cfg_get("modbus:private_max_wait", CFG_PROTO)) * 1000000;
        while(fmb_tty_is_busy(c->tty) && wait_total < max_wait)
        {
            wait_total += rand;
            HAL_Delay(rand/1000);
            if(rand)
                rand *= 2;
            else
            {
                rand = fbs_rand();
                rand = abs(rand - rand / 10 * 10);
                rand *= 1000;
            }
        }
        if(wait_total > max_wait)
        {
            c->pp_send = FALSE;
            fbs_warn(MOD_MODBUS, "send unique id but exceed max wait time", NULL);
            continue;
        }
        pdu_rsp.opcode = 0xA0;
        pdu_rsp.len = 6;
        m = (fmb_manage_t*)pdu_rsp.data;
        m->dev_type = htons(2);
        m->dev_uniqueid = htonl(fbs_uid_get());

        if(c->pp_trans_type == FMB_TRANSPORT_TTY || c->pp_trans_type == FMB_TRANSPORT_RADIO)
            fmb_tty_send(c->tty, c->pp_trans_type, &pdu_rsp, pdu_rsp.len + 2, TRUE);
#ifdef FMB_ENABLE_TCP
        else if(c->pp_trans_type == FMB_TRANSPORT_TCP)
            fmb_tcp_send(c->tcp, c->pp_fd, &pdu_rsp, pdu_rsp.len + 2);
#endif        
        c->pp_send = FALSE;
    }
    return NULL;
}
fmb_core_t *fmb_core_init()
{
    fmb_core_t *c;

    c = fbs_new(fmb_core_t, 1);
    c->running = TRUE;
    c->pp_send = FALSE;
    c->private_proto_thread = fbs_thread_create(__fmb_core_private_proto_thread, c, "private_proto");

#ifdef FMB_ENABLE_TCP
    c->tcp = fmb_tcp_init();
    fmb_tcp_set_recved_cb(c->tcp, &fmb_core_recved, c);
    fmb_tcp_bind(c->tcp, 0, htons(atoi(fbs_cfg_get("modbus:port", CFG_PROTO))));
#endif

    c->tty = fmb_tty_init();
    fmb_tty_set_recved_cb(c->tty, &fmb_core_recved, c);
    return c;
}
int fmb_core_uninit(fmb_core_t *c)
{
    if(c == NULL)
        return FBS_NULL_PARAM;
    c->running = FALSE;
    fmb_tty_uninit(c->tty);
#ifdef FMB_ENABLE_TCP
    fmb_tcp_uninit(c->tcp);
#endif
    fbs_free(c);
    return FBS_SUCC;
}

int fmb_core_recved(void *trans, fmb_transport_type_e trans_type, int fd, void *data, size_t len, bool_t is_broadcast, void *user_data)
{
    int ret = FBS_SUCC;
    fmb_core_t *c;
    fmb_manage_t *m, *curr;
    fmb_pdu_req_t *pdu_req;
    fmb_pdu_rsp_t *pdu_rsp;
    short dot;
    uint32_t offset, total;
    //2是回复包的头，12是8字节时间戳4字节中位数，
	static uint8_t buf[2 + 12 + 50 * 4];
    uint8_t buf_len = 0;

    c = (fmb_core_t*)user_data;
    if(c->cb == NULL)
        return FBS_WRONG_STATE;

    pdu_req = (fmb_pdu_req_t*)data;
    pdu_req->addr = ntohs(pdu_req->addr);
    pdu_req->reg_count = ntohs(pdu_req->reg_count);
    offset = pdu_req->addr % 1000;
    offset *= 2;
    //读取保持寄存器
    if(pdu_req->opcode == 0x3)
    {
        if(is_broadcast)
            return FBS_SUCC;
        dot = pdu_req->addr / 1000;
        if(dot >= 0 && dot < FMB_MAX_CHANNEL)
            ret = c->cb(c, FMB_GET_DATA, &buf[2], dot, 0, 0, 0);
        else if(dot == 6)
        {
            if(c->cb(c, FMB_GET_MANAGE, &buf[2], 0, 0, 0, 0) != FBS_SUCC)
                return FBS_WRONG_STATE;
            if(offset >= sizeof(fmb_manage_t))
                ret = FBS_NOTFOUND;
        }
        else if(dot > 6)
            ret = FBS_NOTFOUND;
        if(offset && offset < 210)
            memmove(&buf[2], &buf[offset + 2], pdu_req->reg_count * 2);
        if(offset > 212)
            ret = FBS_NOTFOUND;
        if(ret == FBS_SUCC)
        {
            pdu_rsp = (fmb_pdu_rsp_t*)buf;
            pdu_rsp->opcode = pdu_req->opcode;
            pdu_rsp->len = pdu_req->reg_count * 2;
            buf_len = pdu_rsp->len + 2;
        }
    }
    //写入保持寄存器
    else if(pdu_req->opcode == 0x10)
    {
        if(c->cb(c, FMB_GET_MANAGE, &buf, 0, 0, 0, 0) != FBS_SUCC)
            return FBS_WRONG_STATE;
        curr = (fmb_manage_t*)&buf;
        if(offset)
            memmove(&pdu_req->data[offset], &pdu_req->data, pdu_req->byte_count);
        m = (fmb_manage_t*)&pdu_req->data;
        m->dev_uniqueid = ntohl(m->dev_uniqueid);
        m->slaveid = ntohs(m->slaveid);
        m->pts = ntohll(m->pts);
        m->out_data_type = ntohs(m->out_data_type);
        m->middle_window = ntohs(m->middle_window);
        m->middle_frq = ntohs(m->middle_frq);
        m->snap_threshold = swap_float_endian(m->snap_threshold);
        m->coefficient = swap_float_endian(m->coefficient);
        m->tty_speed = ntohl(m->tty_speed);
        m->tty_data = ntohs(m->tty_data);
        m->tty_parity = ntohs(m->tty_parity);
        m->tty_stop = ntohs(m->tty_stop);
        m->radio_chn = ntohs(m->radio_chn);
        
        total = offset + pdu_req->byte_count;
        if(offset <= 2 && total >= 8 && curr->slaveid != m->slaveid)
        {
            if(m->dev_uniqueid == fbs_uid_get())
                ret = c->cb(c, FMB_SET_SLAVEID, NULL, m->slaveid, 0, 0, 0);
            else
                ret = FBS_NOTFOUND;
        }
        if(offset <= 8 && total >= 16)
        {
            fbs_time_sync_pts(m->pts);
            printf("sync pts to %llu\n", m->pts);
        }
        if(!is_broadcast && offset <= 16 && total >= 18 && curr->out_data_type != m->out_data_type)
            ret = c->cb(c, FMB_SET_OUT_DATA_TYPE, NULL, m->out_data_type, 0, 0, 0);
        if(!is_broadcast && offset <= 18 && total >= 22 && (curr->middle_frq != m->middle_frq || curr->middle_window != m->middle_window))
            ret = c->cb(c, FMB_SET_MIDDLE_RULE, NULL, m->middle_window, m->middle_frq, 0, 0);
        if(!is_broadcast && offset <= 22 && total >= 26 && curr->snap_threshold != m->snap_threshold)
            ret = c->cb(c, FMB_SET_SNAP_THRESHOLD, NULL, 0, 0, m->snap_threshold, 0);
        if(!is_broadcast && offset <= 26 && total >= 30 && curr->coefficient != m->coefficient)
            ret = c->cb(c, FMB_SET_COEFFICIENT, NULL, 0, 0, m->coefficient, 0);
        if(!is_broadcast && offset <= 30 && total >= 34 && curr->tty_speed != m->tty_speed)
            ret = c->cb(c, FMB_SET_TTY_SPEED, NULL, m->tty_speed, 0, 0, 0);
        if(!is_broadcast && offset <= 34 && total >= 36 && curr->tty_data != m->tty_data)
            ret = c->cb(c, FMB_SET_TTY_DATA, NULL, m->tty_data, 0, 0, 0);
        if(!is_broadcast && offset <= 36 && total >= 38 && curr->tty_parity != m->tty_parity)
            ret = c->cb(c, FMB_SET_TTY_PARITY, NULL, m->tty_parity, 0, 0, 0);
        if(!is_broadcast && offset <= 38 && total >= 40 && curr->tty_stop != m->tty_stop)
            ret = c->cb(c, FMB_SET_TTY_STOP, NULL, m->tty_stop, 0, 0, 0);
        if(!is_broadcast && offset <= 40 && total >= 42 && curr->radio_chn != m->radio_chn)
            ret = c->cb(c, FMB_SET_RADIO_CHN, NULL, m->radio_chn, 0, 0, 0);

        //广播包不回复
        if(is_broadcast)
            return FBS_SUCC;
        if(ret == FBS_SUCC)
        {
            pdu_req->addr = htons(pdu_req->addr);
            pdu_req->reg_count = htons(pdu_req->reg_count);
            memcpy(buf, pdu_req, 5);
            buf_len = 5;
        }
    }
    //私有部分
    else if(pdu_req->opcode == 0xA0)
    {
        if(pdu_req->addr == 0x7000)
        {
            if(!c->pp_send)
            {
                c->pp_send = TRUE;
                c->pp_trans_type = trans_type;
                c->pp_fd = fd;
            }
            return FBS_SUCC;
        }
    }
    if(ret != FBS_SUCC)
    {
        pdu_req->opcode = 0x90;
        if(ret == FBS_EXCEED_PARAM)
            pdu_req->addr = 0x03;
        else if(ret == FBS_NOTFOUND)
            pdu_req->addr = 0x02;
        memcpy(buf, pdu_req, 2);
        buf_len = 2;
    }
    if(trans_type == FMB_TRANSPORT_TTY || trans_type == FMB_TRANSPORT_RADIO)
        fmb_tty_send(trans, trans_type, buf, buf_len, TRUE);
#ifdef FMB_ENABLE_TCP
    else if(trans_type == FMB_TRANSPORT_TCP)
        fmb_tcp_send(trans, fd, buf, buf_len);
#endif
    return FBS_SUCC;
}
int fmb_core_set_cb(fmb_core_t *c, int (*cb)(fmb_core_t*, enum fmb_cb_type, void *buf, int int1, int int2, float f1, float f2), void *user_data)
{
    if(c == NULL)
        return FBS_NULL_PARAM;
    c->cb = cb;
    return FBS_SUCC;
}
int fmb_core_set_slaveid(uint8_t id)
{
    int ret;
    char buf[64];

    sprintf(buf, "%u", id);
    ret = fbs_cfg_set("modbus:slaveid", buf, CFG_PROTO);
    fbs_cfg_save(CFG_PROTO);
    return ret;
}
int fmb_core_set_tty_speed(fmb_core_t *c, int speed)
{
    return fmb_tty_set_speed(c->tty, speed);
}
int fmb_core_set_tty_data(fmb_core_t *c, int data)
{
    return fmb_tty_set_data(c->tty, data);
}
int fmb_core_set_tty_parity(fmb_core_t *c, char parity)
{
    return fmb_tty_set_parity(c->tty, parity);
}
int fmb_core_set_tty_stop(fmb_core_t *c, int stop)
{
    return fmb_tty_set_stop(c->tty, stop);
}
int fmb_core_set_radio_chn(fmb_core_t *c, uint16_t chn)
{
    return fmb_tty_set_chn(c->tty, chn);
}