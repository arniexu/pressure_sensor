#ifndef FMB_CORE
#define FMB_CORE
#include "f2f_modbus/f2f_modbus.h"

enum fmb_cb_type
{
	FMB_GET_DATA = 1,
	FMB_GET_MANAGE = 2,
	FMB_SET_OUT_DATA_TYPE = 200,
	FMB_SET_MIDDLE_RULE,
	FMB_SET_SNAP_THRESHOLD,
	FMB_SET_COEFFICIENT,
	FMB_SET_SLAVEID,
	FMB_SET_TTY_SPEED,
	FMB_SET_TTY_DATA,
	FMB_SET_TTY_PARITY,
	FMB_SET_TTY_STOP,
	FMB_SET_RADIO_CHN
};
enum fmb_transport_type
{
	FMB_TRANSPORT_TCP = 1,
	FMB_TRANSPORT_TTY = 2,
	FMB_TRANSPORT_RADIO = 3
};
struct fmb_pdu_req
{
	uint8_t opcode		:8;
	uint16_t addr		:16;
	uint16_t reg_count	:16;
	uint8_t byte_count	:8;
	uint8_t data[256];
};
struct fmb_pdu_rsp
{
	uint8_t opcode		:8;
	uint8_t len			:8;
	uint8_t data[256];
};

//这句估计兼容性有问题，在Linux上，是告诉编译器以1bit作为内存对其的最小单位
//这个非常重要，如果这个结构体的长度因为float或者uint64_t成员的存在，而采用64bit对齐的话
//拼出来的数据包是完全不对的
//检测内存对齐也很简单，sizeof这个结构体，如果长度正好是42字节，那就是对的了
#pragma pack(1)
struct fmb_manage
{
	//2:deflection 3:strain
	uint16_t dev_type		:16;
	uint32_t dev_uniqueid;
	uint16_t slaveid		:16;
	uint64_t pts;
	uint16_t out_data_type	:16;
	uint16_t middle_window	:16;
	uint16_t middle_frq		:16;
	float snap_threshold;
	float coefficient;
	uint32_t tty_speed;
	uint16_t tty_data		:16;
	uint16_t tty_parity		:16;
	uint16_t tty_stop		:16;
	uint16_t radio_chn		:16;
};
#pragma pack()

struct fmb_core
{
	bool_t running;

#ifdef FMB_ENABLE_TCP
	fmb_tcp_t *tcp;
#endif
	fmb_tty_t *tty;
	int (*cb)(fmb_core_t*, enum fmb_cb_type, void *buf, int int1, int int2, float f1, float f2);

	bool_t pp_send;
	fmb_transport_type_e pp_trans_type;
	int pp_fd;
	pthread_t private_proto_thread;
};

#ifdef __cplusplus
extern "C" {
#endif


	fmb_core_t *fmb_core_init();
	int fmb_core_uninit(fmb_core_t *c);

	int fmb_core_recved(void *trans, fmb_transport_type_e trans_type, int fd, void *data, size_t len, bool_t is_broadcast, void *user_data);
	int fmb_core_set_cb(fmb_core_t *c, int (*cb)(fmb_core_t*, enum fmb_cb_type, void *buf, int int1, int int2, float f1, float f2), void *user_data);
	int fmb_core_set_slaveid(uint8_t id);

	int fmb_core_set_tty_speed(fmb_core_t *c, int speed);
	int fmb_core_set_tty_data(fmb_core_t *c, int data);
	int fmb_core_set_tty_parity(fmb_core_t *c, char parity);
	int fmb_core_set_tty_stop(fmb_core_t *c, int stop);
	int fmb_core_set_radio_chn(fmb_core_t *c, uint16_t chn);

#ifdef __cplusplus
}
#endif
#endif
