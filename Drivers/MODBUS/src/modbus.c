#include "fmb_core.h"
#include "fmb_tty.h"
#include "f2f_modbus.h"
#include "modbus.h"
#include "main.h"

bool_t __loop = TRUE;

int hisi_get_pipe_data(int chn, void *buf)
{
	if(chn != 0)
		return FBS_EXCEED_PARAM;
	//chn代表了取哪个通道的数据，在本设备上一定为0，如果不为0应该返回FBS_EXCEED_PARAM
	//buf长度是8 + 4 + 50 * 4字节，类型uint8_t
	//0-7字节应当存储经过htonll转换后的时间戳。
	//8-11字节应当为0
	//从12字节开始，每4个字节存储一个经过swap_float_endian转换的ADC数据。
	//根据out_data_type的设置不同，这个ADC数据可能是原始数据，也可能是原始数据乘以coefficient系数
	//上述的时间戳，必须是第12个字节这个数据采集时的时间。
	//程序走到这个地方的时间是不确定的，外部设备有可能1s过来轮训一次，也有可能是200ms、500ms
	//所以程序需要维护一个固定长度的缓冲区，缓存2s的数据，超出2s则始终向最旧的位置覆盖数据
	//本设备被轮训的时候，始终在缓冲区最旧的位置取数据。这个缓冲区应该采用循环链表的方式实现
	return FBS_SUCC;
}
void hisi_get_manage_data(void *buf)
{
	fmb_manage_t *m = NULL;
	if(buf == NULL)
		return;

	//这里需要实现获得管理数据的功能
	//下面的程序中，需要把正确的配置信息等填充到结构体的相应位置中去
	m = (fmb_manage_t*)buf;
	m->dev_uniqueid = htonl(fbs_uid_get());

	m->dev_type = htons(3);
	
	m->pts = hisi_get_curr_pts();
	m->pts = htonll(m->pts);

	m->out_data_type = (uint16_t)atoi(fbs_cfg_get("modbus:out_data_type", CFG_PROTO));
	m->out_data_type = htons(m->out_data_type);
	
	m->coefficient = atof(fbs_cfg_get("modbus:coefficient", CFG_PROTO));
	m->coefficient = swap_float_endian(m->coefficient);

	m->slaveid = (uint16_t)atoi(fbs_cfg_get("modbus:slaveid", CFG_PROTO));
	m->slaveid = htons(m->slaveid);

	m->tty_speed = (uint32_t)atoi(fbs_cfg_get("modbus:tty_speed", CFG_PROTO));
	m->tty_speed = htonl(m->tty_speed);

	m->tty_data = (uint16_t)atoi(fbs_cfg_get("modbus:tty_data", CFG_PROTO));
	m->tty_data = htons(m->tty_data);

	m->tty_parity = (uint16_t)(fbs_cfg_get("modbus:tty_parity", CFG_PROTO)[0]);
	m->tty_parity = htons(m->tty_parity);

	m->tty_stop = (uint16_t)atoi(fbs_cfg_get("modbus:tty_stop", CFG_PROTO));
	m->tty_stop = htons(m->tty_stop);

	m->radio_chn = (uint16_t)atoi(fbs_cfg_get("modbus:radio_chn", CFG_PROTO));
	m->radio_chn = htons(m->radio_chn);
}

int hisi_set_out_data_type(fmd_filter_pipe_data_type_e type)
{
    char buf[16];

    sprintf(buf, "%d", (int)type);
    return fbs_cfg_set("modbus:out_data_type", buf, CFG_PROTO);
}
int hisi_set_coefficient(float coefficient)
{
    char buf[16];

    sprintf(buf, "%f", coefficient);
    return fbs_cfg_set("modbus:coefficient", buf, CFG_PROTO);
}
uint64_t hisi_get_curr_pts()
{
    //返回当前us单位的时间戳，务必实现
    return 0;
}

int fmb_cb(fmb_core_t *c, enum fmb_cb_type cb_type, void *buf, int int1, int int2, float f1, float f2)
{
	switch(cb_type)
	{
		case FMB_GET_DATA:
			return hisi_get_pipe_data(int1, buf);
		case FMB_GET_MANAGE:
			hisi_get_manage_data(buf);
			break;
			//设置数据的输出类型，也就是输出原始数据还是乘以系数
		case FMB_SET_OUT_DATA_TYPE:
			printf("FMB_SET_OUT_DATA_TYPE to %d\n", int1);
			return hisi_set_out_data_type((fmd_filter_pipe_data_type_e)int1);
			//设置系数
		case FMB_SET_COEFFICIENT:
			printf("FMB_SET_COEFFICIENT to %f\n", f1);
			return hisi_set_coefficient(f1);
		case FMB_SET_SLAVEID:
			printf("FMB_SET_SLAVEID to %d\n", int1);
			return fmb_core_set_slaveid((uint8_t)int1);
		case FMB_SET_TTY_SPEED:
			printf("FMB_SET_TTY_SPEED to %d\n", int1);
			return fmb_core_set_tty_speed(c, int1);
		case FMB_SET_TTY_DATA:
			printf("FMB_SET_TTY_DATA to %d\n", int1);
			return fmb_core_set_tty_data(c, int1);
		case FMB_SET_TTY_PARITY:
			printf("FMB_SET_TTY_PARITY to %c\n", (char)int1);
			return fmb_core_set_tty_parity(c, (char)int1);
		case FMB_SET_TTY_STOP:
			printf("FMB_SET_TTY_STOP to %d\n", int1);
			return fmb_core_set_tty_stop(c, int1);
		case FMB_SET_RADIO_CHN:
			printf("FMB_SET_RADIO_CHN to %d\n", int1);
			return fmb_core_set_radio_chn(c, int1);
		default:
			break;
	}
	return FBS_SUCC;
}
int modbus(int argc, char *argv[])
{
	fmb_core_t *mb;

	//这里调用fmb库的初始化
	mb = fmb_core_init();
	if(mb == NULL)
	{
		fbs_err(MOD_CORE, "failed to init modbus server!", NULL);
		return FBS_SYS_CALL_FAILED;
	}
	//把本文件的fmb_cb作为回调函数给fmb库，用于响应fmb库对外的事件
	fmb_core_set_cb(mb, &fmb_cb, NULL);

	while(__loop)
	{
		HAL_Delay(2);
	}

	fmb_core_uninit(mb);
  exit(0);
}
