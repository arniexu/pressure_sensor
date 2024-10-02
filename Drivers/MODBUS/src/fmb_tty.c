#include "fmb_tty.h"
#include "main.h"

extern RNG_HandleTypeDef hrng;

extern SPI_HandleTypeDef hspi1;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;


static uint16_t calcrc(uint8_t *buf, int len)
{
    const uint16_t table[256] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601,
        0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0,
        0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81,
        0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941,
        0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01,
        0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0,
        0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081,
        0x1040, 0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 0x3C00,
        0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0,
        0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 0x2800, 0xE8C1, 0xE981,
        0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41,
        0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700,
        0xE7C1, 0xE681, 0x2640, 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0,
        0x2080, 0xE041, 0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281,
        0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 0xAA01,
        0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1,
        0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01, 0x7EC0, 0x7F80,
        0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541,
        0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101,
        0x71C0, 0x7080, 0xB041, 0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0,
        0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481,
        0x5440, 0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, 0x8801,
        0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1,
        0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 0x4400, 0x84C1, 0x8581,
        0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341,
        0x4100, 0x81C1, 0x8081, 0x4040
    };
    uint16_t crc = 0xffff;

    for (int i = 0; i < len; i++) {
        crc = (crc >> 8) ^ table[(crc ^ buf[i]) & 0xFF];
    }

    return crc;
}
static void *__fmb_tty_server_thread(void *arg)
{
	// fd_set set; 没有poll-select机制
	uint16_t crc = 0;
	fmb_transport_type_e cb_type;
	// int nfound = 0;
	// int maxfd; 没有poll-select机制
	uint32_t len = 0, read_len = 0;
	uint8_t *buf = NULL, tty_buf[FMB_TTY_BUF_LEN];
	// radio_buf[RADIO_MTU + 2] = {0};
	// struct timeval tv; stm32有自己的超时机制
	fmb_pdu_req_t *pdu_req;
	uint32_t tv_usec;
	modbus_rtu_pack_t *rtu;
	bool_t recved_data = FALSE;
	fmb_tty_t *tty = (fmb_tty_t*)arg;

    tv_usec = fmb_tty_get_interval();
	while(tty->running)
	{
		//下面是标准的linux中的select机制，非阻塞等待fd的动作。
		//如果stm32上没有相应的函数，下面标注了哪些地方是具体的实现体
		// FD_ZERO(&set);
		// if(tty->tty_fd)
		// 	FD_SET(tty->tty_fd, &set);
		// if(tty->radio_fd)
		//     FD_SET(tty->radio_fd, &set);

		recved_data = FALSE;
		// tv.tv_sec = 0;
		// tv.tv_usec = tv_usec;
		// maxfd = tty->tty_fd > tty->radio_fd ? tty->tty_fd : tty->radio_fd;
		// if((nfound = select(maxfd + 1, &set, (fd_set *)0, (fd_set *)0, tv.tv_sec || tv.tv_usec ? &tv : NULL)) < 0)
		// {
		// 	return NULL;
		// }
		// else if(nfound == 0)
		// {
		// 	tty->tty_busy = FALSE;
		// 	tty->radio_busy = FALSE;
		// 	continue;
		// }
        //radio这部分是通过无线电进行通讯的，一定要把代码保留下来
        //接下来这个传感器有可能做无线的
        #if 0
		if(FD_ISSET(tty->radio_fd, &set))
		{
            nfound--;
            tty->radio_busy = TRUE;
            len = read(tty->radio_fd, radio_buf, RADIO_MTU + 2);
            if(len <= 0)
            {
                usleep(1000);
                return FALSE;
            }

            //从这里往后，是radio收到消息的处理函数。
            //radio收到len长度的消息，放到radio_buf中，最长RADIO_MTU + 2个字节
            //然后进行下列处理。如果能够采用select，下列语句保持注释，如果要单独写，要把radio_busy设置为TRUE
            //tty->radio_busy = TRUE;
            if(radio_buf[0] != 0xff || radio_buf[1] < 1 || radio_buf[1] > 128)
                return FALSE;
            if(len <= 0)
                return FALSE;

            //radio_buf[0]: 0xff
            //radio_buf[1]: data len
            //radio_buf[2]: data[0]
            if(radio_buf[2] == 0 && radio_buf[3] == 0 && radio_buf[4] == 0 && radio_buf[5] == 0)
            {
                tty->rxlen = (radio_buf[6] << 8) | radio_buf[7];
                tty->rxlen = tty->rxlen > RADIO_MAX_PACKET ? RADIO_MAX_PACKET : tty->rxlen;
                tty->rx_wptr = tty->rxbuf;
                memcpy(tty->rx_wptr, radio_buf + 8, radio_buf[1] - 6);
                tty->rx_wptr += (radio_buf[1] - 6);
                tty->total_rx_bytes += 6;
                tty->total_rx_packets++;
                continue;
            }
            else if(tty->rx_wptr != tty->rxbuf)
            {
                memcpy(tty->rx_wptr, radio_buf + 2, radio_buf[1]);
                tty->rx_wptr += radio_buf[1];
                if((tty->rx_wptr - tty->rxbuf) >= tty->rxlen)
                {
                    len = tty->rxlen;
                    buf = tty->rxbuf;
                    cb_type = FMB_TRANSPORT_RADIO;
                    recved_data = TRUE;
                    tty->total_rx_bytes += tty->rxlen;

                    tty->rx_wptr = tty->rxbuf;
                    tty->rxlen = 0;
                    tty->total_rx_packets++;
                }
            }
            else
            {
                len = radio_buf[1];
                buf = &radio_buf[2];
                cb_type = FMB_TRANSPORT_RADIO;
                recved_data = TRUE;
                tty->total_rx_bytes += len;
                tty->total_rx_packets++;
            }
            //radio处理到这里结束
        }
        #endif
		// if(FD_ISSET(tty->tty_fd, &set))
		{
			// nfound--;
			tty->tty_busy = TRUE;
			if((HAL_OK == HAL_UART_Receive(&huart2, tty_buf + len, FMB_TTY_BUF_LEN - len, HAL_MAX_DELAY)) < 0)
			{
				fbs_err(MOD_SERIAL, "receive data error fd:%d", tty->tty_fd);
				continue;
			}
			len += read_len;
			if(read_len == 0)
			{
				fbs_err(MOD_SERIAL, "disconnect fd:%d", tty->tty_fd);
				tty->tty_fd = 0;
				continue;
			}
            //tty也就是485的处理这里开始
            //tty接收了read_len长度的数据，放在tty_buf + len的位置，最长FMB_TTY_BUF_LEN - len字节长度
            //接收完成后len += read_len。上述为了完成拼包
            //如果能够采用select，下列语句保持注释，如果要单独写，要把tty_busy设置为TRUE
            //tty->tty_busy = TRUE;
            buf = tty_buf;
            cb_type = FMB_TRANSPORT_TTY;
            recved_data = TRUE;
            //tty处理在这里结束
        }
        //下面的if实现了数据包的处理，务必要原封不动的实现
        if(recved_data)
        {
            //数据是否收全了
			if(len < 6)
				continue;
            pdu_req = (fmb_pdu_req_t*)&buf[1];
            if(pdu_req->opcode == 0x10 && len < (pdu_req->byte_count + 2 + 7))
                continue;
                
            //判断CRC校验是否正确
			crc = calcrc(buf, len - 2);
			crc = htons(crc);
			if(buf[len - 1] != (crc & 0xff) || buf[len - 2] != ((crc >> 8) & 0xff))
			{
				len = 0;
				continue;
			}
            //判断是否是发给我的，同时接受广播包
			if(buf[0] != 0x0 && buf[0] != atoi(fbs_cfg_get("modbus:slaveid", CFG_PROTO)))
			{
				len = 0;
				continue;
			}
            rtu = (modbus_rtu_pack_t*)buf;
			if(tty->cb)
				tty->cb(tty, cb_type, 0, buf + 1, len - 3, rtu->addr == 0, tty->user_data);
			len = 0;
		}
	}

    return NULL;
}
static bool_t __fmb_tty_start(fmb_tty_t *tty)
{
    //这里是打开串口的操作，fbs_serial_open最终返回一个fd
    if(fbs_cfg_get("modbus:tty_dev", CFG_PROTO) && fbs_cfg_get("modbus:tty_speed", CFG_PROTO) &&
        fbs_cfg_get("modbus:tty_data", CFG_PROTO) && fbs_cfg_get("modbus:tty_parity", CFG_PROTO) &&
        fbs_cfg_get("modbus:tty_stop", CFG_PROTO))
    {
        tty->tty_fd = fbs_serial_open(
            fbs_cfg_get("modbus:tty_dev", CFG_PROTO), 
            atoi(fbs_cfg_get("modbus:tty_speed", CFG_PROTO)), 
            atoi(fbs_cfg_get("modbus:tty_data", CFG_PROTO)), 
            fbs_cfg_get("modbus:tty_parity", CFG_PROTO)[0], 
            atoi(fbs_cfg_get("modbus:tty_stop", CFG_PROTO)));
    }
    if(tty->tty_fd <= 0)
        return FALSE;

    //这里是打开了GPIO口的设备，我们的电路图已经采用单独的GPIO控制RE、DE脚位，从而控制485的收发
    //这个务必实现
    // tty->gpio_fd = open("/dev/gpio", O_RDWR);
    // if(tty->gpio_fd <= 0)
    // {
    //     close(tty->tty_fd);
    //     return FALSE;
    // }
	// if(ioctl(tty->gpio_fd, GPIO_CMD_SET_485, &value) < 0)
	// 	fbs_err(MOD_MEDIA, "set si446x clear state", NULL);
    if(tty->tty_fd > 0)
        fbs_msg(MOD_MODBUS, "open %s ok!", fbs_cfg_get("modbus:tty_dev", CFG_PROTO));
    return tty->tty_fd > 0;
}
static bool_t __fmb_radio_start(fmb_tty_t *tty)
{
	#if 0
    int fd;
    uint16_t chn;

    return FALSE;

    if(fbs_cfg_get("modbus:radio_dev", CFG_PROTO) == NULL)
        return FALSE;
    if(fbs_cfg_get("modbus:radio_chn", CFG_PROTO) == NULL)
        return FALSE;
	fd = open(fbs_cfg_get("modbus:radio_dev", CFG_PROTO), O_RDWR);
	if(fd <= 0)
	{
		fbs_err(MOD_MEDIA, "open %s failed! %s", fbs_cfg_get("modbus:radio_dev", CFG_PROTO), strerror(errno));
		return FALSE;
	}
	tty->radio_fd = fd;
	tty->rxlen = 0;
	memset(tty->rxbuf, 0, RADIO_MAX_PACKET);
	tty->rx_wptr = tty->rxbuf;

	if(ioctl(tty->radio_fd, SI446X_CMD_CLEAR_STATE, NULL) < 0)
		fbs_err(MOD_MEDIA, "set si446x clear state", NULL);

    chn = atoi(fbs_cfg_get("modbus:radio_chn", CFG_PROTO));
	if(ioctl(tty->radio_fd, SI446X_CMD_SET_CHANNEL, &chn) < 0)
		fbs_err(MOD_MEDIA, "set si446x set channel", NULL);

    if(tty->radio_fd > 0)
        fbs_msg(MOD_MODBUS, "open %s ok! fd:%d", fbs_cfg_get("modbus:radio_dev", CFG_PROTO), tty->radio_fd);
    return tty->radio_fd > 0;
		#endif
		return 0;
}
fmb_tty_t *fmb_tty_init()
{
    bool_t anyone = FALSE;
    fmb_tty_t *tty;

    tty = fbs_new(fmb_tty_t, 1);
	tty->running = TRUE;
    tty->tty_busy = FALSE;
    tty->radio_busy = FALSE;
    if(__fmb_tty_start(tty))
        anyone = TRUE;
    if(__fmb_radio_start(tty))
        anyone = TRUE;

    if(anyone && tty->thread == 0)
        tty->thread = fbs_thread_create(__fmb_tty_server_thread, tty, "tty_server_thread");
    return tty;
}
static void __fmb_tty_stop(fmb_tty_t *tty)
{
	#if 0
	tty->running = FALSE;

	if(tty->radio_fd)
    {
        if(ioctl(tty->radio_fd, SI446X_CMD_PRINT_STATE, NULL) < 0)
            fbs_err(MOD_MEDIA, "print si446x state", NULL);
		close(tty->radio_fd);
    }
    if(tty->tty_fd)
        close(tty->tty_fd);
    if(tty->gpio_fd)
        close(tty->gpio_fd);
    if(tty->thread)
        fbs_thread_join(tty->thread, NULL);
	#endif
}
int fmb_tty_uninit(fmb_tty_t *tty)
{
    if(tty == NULL)
        return FBS_NULL_PARAM;

    __fmb_tty_stop(tty);
    fbs_free(tty);
    return FBS_SUCC;
}
int fmb_tty_send(fmb_tty_t *tty, fmb_transport_type_e type, void *data, size_t len, bool_t is_rsp)
{
		int value = 0;
    int i = 0, tty_speed = atoi(fbs_cfg_get("modbus:tty_speed", CFG_PROTO));
    uint8_t *buf;
		uint16_t crc;
    modbus_rtu_pack_t *pack;

    buf = fbs_new(uint8_t, len + 3);
    pack = (modbus_rtu_pack_t*)buf;
    if(is_rsp)
    {
	    pack->addr = atoi(fbs_cfg_get("modbus:slaveid", CFG_PROTO));
        memcpy(pack->pdu, data, len);
        crc = calcrc(buf, len + 1);
        buf[len + 1] = crc & 0xff;
        buf[len + 2] = (crc >> 8) & 0xff;
        len += 3;
    }
    else
    {
        memcpy(buf, data, len);
        crc = calcrc(buf, len);
        buf[len] = crc & 0xff;
        buf[len + 1] = (crc >> 8) & 0xff;
        len += 2;
    }

    //串口的发送
    if(type == FMB_TRANSPORT_TTY)
    {
        //先把DE、RE拉高
        value = 1;
				//@TODO: 
        //if(HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET) != HAL_OK)
            fbs_err(MOD_MEDIA, "set si446x clear state", NULL);
        //串口发送数据
        i = HAL_UART_Transmit(&huart2, buf, len, HAL_MAX_DELAY);

        //DE、RE拉低，进入接收模式
        value = 0;
				//@TODO: 
        //if(ioctl(tty->gpio_fd, GPIO_CMD_SET_485, &value) < 0)
            fbs_err(MOD_MEDIA, "set si446x clear state", NULL);
    }
		#if 0
    else if(type == FMB_TRANSPORT_RADIO)
    {
        ptr = head = buf;
        if(len > RADIO_MTU)
        {
            memset(buf1, 0, RADIO_MTU);
            buf1[0] = 0;
            buf1[1] = 0;
            buf1[2] = 0;
            buf1[3] = 0;
            buf1[4] = (len >> 8) & 0xff;
            buf1[5] = len & 0xff;
            memcpy(buf1 + 6, ptr, RADIO_MTU - 6);
            ptr += (RADIO_MTU - 6);
            write(tty->radio_fd, buf1, RADIO_MTU);

            while((ptr - head) < len)
            {
                len1 = len - (ptr - head);
                len1 = len1 > RADIO_MTU ? RADIO_MTU : len1;
                i = write(tty->radio_fd, ptr, len1);
                ptr += len1;
            }
        }
        else
        {
            i = write(tty->radio_fd, ptr, len);
            tty->total_tx_bytes += len;
            tty->total_tx_packets++;
        }
    }
		#endif
    fbs_free(buf);
    return i;
}
int fmb_tty_set_recved_cb(fmb_tty_t *tty, int (*cb)(void *tty, fmb_transport_type_e trans, int fd, void *data, size_t len, bool_t is_broadcast, void *user_data), void *user_data)
{
    if(tty == NULL)
        return FBS_NULL_PARAM;
    tty->cb = cb;
	tty->user_data = user_data;
    return FBS_SUCC;
}
int fmb_tty_set_speed(fmb_tty_t *tty, int speed)
{
    int ret;
    char buf[64];

    if(speed != 115200 && speed != 38400 && speed != 19200 && speed != 9600 && speed != 4800 && speed != 2400 && speed != 1200 && speed != 300)
    {
        fbs_err(MOD_MODBUS, "set tty speed %d not allowed", speed);
        return FBS_EXCEED_PARAM;
    }
    __fmb_tty_stop(tty);
    sprintf(buf, "%d", speed);
    ret = fbs_cfg_set("modbus:tty_speed", buf, CFG_PROTO);
    fbs_cfg_save(CFG_PROTO);
    __fmb_tty_start(tty);
    return ret;
}
int fmb_tty_set_data(fmb_tty_t *tty, int data)
{
    int ret;
    char buf[64];

    if(data != 7 && data != 8)
    {
        fbs_err(MOD_MODBUS, "set tty data %d not allowed", data);
        return FBS_EXCEED_PARAM;
    }
    __fmb_tty_stop(tty);
    sprintf(buf, "%d", data);
    ret = fbs_cfg_set("modbus:tty_data", buf, CFG_PROTO);
    fbs_cfg_save(CFG_PROTO);
    __fmb_tty_start(tty);
    return ret;
}
int fmb_tty_set_parity(fmb_tty_t *tty, char parity)
{
    int ret;
    char buf[64];

    if(parity != 'n' && parity != 'N' && parity != 'o' && parity != 'O' && 
        parity != 'e' && parity != 'E' && parity != 's' && parity != 'S')
    {
        fbs_err(MOD_MODBUS, "set tty parity %c not allowed", parity);
        return FBS_EXCEED_PARAM;
    }
    __fmb_tty_stop(tty);
    sprintf(buf, "%c", parity);
    ret = fbs_cfg_set("modbus:tty_parity", buf, CFG_PROTO);
    fbs_cfg_save(CFG_PROTO);
    __fmb_tty_start(tty);
    return ret;
}
int fmb_tty_set_stop(fmb_tty_t *tty, int stop)
{
    int ret;
    char buf[64];

    if(stop != 1 && stop != 2)
    {
        fbs_err(MOD_MODBUS, "set tty stop %d not allowed", stop);
        return FBS_EXCEED_PARAM;
    }
    __fmb_tty_stop(tty);
    sprintf(buf, "%d", stop);
    ret = fbs_cfg_set("modbus:tty_stop", buf, CFG_PROTO);
    fbs_cfg_save(CFG_PROTO);
    __fmb_tty_start(tty);
    return ret;
}
int fmb_tty_set_chn(fmb_tty_t *tty, uint16_t chn)
{
	    int ret = 0;
	#if 0
    char buf[16];

    if(chn < 0 || chn > 63)
        return FBS_EXCEED_PARAM;
    sprintf(buf, "%u", chn);
    ret = fbs_cfg_set("modbus:radio_chn", buf, CFG_PROTO);
    fbs_cfg_save(CFG_PROTO);
    if(tty->radio_fd <= 0)
        return ret;
    if(ioctl(tty->radio_fd, SI446X_CMD_SET_CHANNEL, &chn) < 0)
        fbs_err(MOD_MEDIA, "set si446x set channel", NULL);
  #endif
	return ret;
}
uint32_t fmb_tty_get_interval()
{
    return 1000000 / atoi(fbs_cfg_get("modbus:tty_speed", CFG_PROTO)) * 
        (1 + atoi(fbs_cfg_get("modbus:tty_data", CFG_PROTO)) + atoi(fbs_cfg_get("modbus:tty_stop", CFG_PROTO))) * 4;
}
bool_t fmb_tty_is_busy(fmb_tty_t *tty)
{
    return tty->tty_busy | tty->radio_busy;
}
