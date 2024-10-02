#ifndef F2F_MODBUS
#define F2F_MODBUS

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#ifndef WIN32
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

typedef enum
{
	CFG_GENERAL = 0,
	CFG_MEDIA,
	CFG_LAYOUT,
	CFG_PROTO,
	CFG_NET,
	CFG_SCREEN,
	CFG_VO,
	CFG_VI,
	CFG_AO,
	CFG_AI,
	CFG_RTSP,
	CFG_LAMP,
	CFG_AUDIO,
	CFG_VIDEO,
	CFG_PERSONAL
}fbs_cfg_type_e;

typedef enum
{
	MOD_MEM = 1,
	MOD_THREAD,
	MOD_MUTEX,
	MOD_LIST,
	MOD_CFG,
	MOD_INI,
	MOD_DB,
	MOD_SERIAL,
	MOD_STUN,
	MOD_MEDIA,
	MOD_ENV,
	MOD_RTSP,
	MOD_MODBUS,
	MOD_PROTO,
	MOD_MHL,
	MOD_STOR,
	MOD_CORE,
	MOD_APP
}fbs_log_module;

typedef enum
{
	LOG_NOLOG = 1,
	LOG_CRIT,
	LOG_ERR,
	LOG_WARN,
	LOG_MSG,
	LOG_DBG
}fbs_log_level;

#define MODBUS_REG_BASE_ADDR 0x0
#define MODBUS_REG_TTY_SPEED_OFFSET 0x0
#define MODBUS_REG_TTY_DATA_OFFSET 0x40
#define MODBUS_REG_TTY_PARITY_OFFSET 0x80
#define MODBUS_REG_TTY_STOP_OFFSET 0xC0
#define MODBUS_REG_TTY_FD_OFFSET 0x100
#define MODBUS_REG_TTY_DEV_OFFSET 0x140
#define MODBUS_REG_SLAVEID_OFFSET 0x180


#define	fbs_log(level, module, fmt, args...)	_fbs_log(level, module, __FILE__, __LINE__, fmt, args)
#define	fbs_crit(module, fmt, args1...)			fbs_log(LOG_CRIT, module, fmt, args1)
#define	fbs_err(module, fmt, args1...)			fbs_log(LOG_ERR, module, fmt, args1)
#define	fbs_warn(module, fmt, args1...)			fbs_log(LOG_WARN, module, fmt, args1)
#define	fbs_msg(module, fmt, args1...)			fbs_log(LOG_MSG, module, fmt, args1)
#define	fbs_dbg(module, fmt, args1...)			fbs_log(LOG_DBG, module, fmt, args1)


#define bool_t				    unsigned char
#define TRUE					1
#define FALSE					0
#define RADIO_MTU			    128
#define RADIO_MAX_PACKET	    4090

#define FBS_SUCC				0
#define FBS_NULL_PARAM			-1
#define FBS_EXCEED_PARAM		-2
#define FBS_WRONG_STATE			-3
#define FBS_NO_ACTION			-4
#define FBS_NO_FILE				-5
#define FBS_DUPLICATE			-6
#define FBS_NOTFOUND			-7
#define FBS_BUSY				-8
#define FBS_SYS_CALL_FAILED		-9
#define FBS_HISI_FAILED			-10
#define FBS_NOMEM				-11
#define FBS_UNDEFINED_ERROR		-12

#define FMB_MAX_CHANNEL         1


#define 	fbs_new(type, count)	(type*)malloc(sizeof(type) * count)
#define 	fbs_free(ptr)			free(ptr)

enum fmd_filter_pipe_data_type
{
	FMD_PIPE_INVALID = -1,
	FMD_PIPE_ORIGINAL = 0,
	FMD_PIPE_ORI_COEFFICIENT = 1,
	FMD_PIPE_MIDDLE = 2,
	FMD_PIPE_MID_COEFFICIENT = 3,
};
typedef enum fmd_filter_pipe_data_type fmd_filter_pipe_data_type_e;

#ifdef FMB_ENABLE_TCP
typedef struct modbus_tcp_pack          modbus_tcp_pack_t;
typedef struct fmb_tcp_data             fmb_tcp_data_t;
typedef struct fmb_tcp					fmb_tcp_t;
#endif
typedef struct modbus_rtu_pack          modbus_rtu_pack_t;
typedef struct fmb_pdu_req              fmb_pdu_req_t;
typedef struct fmb_pdu_rsp              fmb_pdu_rsp_t;
typedef struct fmb_manage               fmb_manage_t;
typedef struct fmb_tty					fmb_tty_t;
typedef struct fmb_core					fmb_core_t;

typedef enum fmb_transport_type         fmb_transport_type_e;
typedef enum fmb_cb_type				fmb_cb_type_e;


const char *fbs_cfg_get(const char *key, fbs_cfg_type_e type);
int fbs_cfg_set(const char *key, const char *value, fbs_cfg_type_e type);
int fbs_cfg_save(fbs_cfg_type_e type);
int fbs_rand();
uint32_t fbs_uid_get();
void fbs_time_sync_pts(uint64_t pts);
int fbs_serial_open(const char *dev, int speed, int databits, char parity, int stopbits);
pthread_t fbs_thread_create(void *(*func)(void *), void *arg, const char *name);

float swap_float_endian(float data); 
uint64_t htonll(uint64_t val);
uint64_t ntohll(uint64_t val);

#ifdef FMB_ENABLE_TCP
#include "f2f_modbus/fmb_tcp.h"
#endif
#include "f2f_modbus/fmb_tty.h"
#include "f2f_modbus/fmb_core.h"
#endif
