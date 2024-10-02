#ifndef FMB_TTY
#define FMB_TTY
#include "f2f_modbus/f2f_modbus.h"

#define FMB_TTY_BUF_LEN		1024
#define SI446X_CMD_CLEAR_STATE		0x10
#define SI446X_CMD_PRINT_STATE		0x20
#define SI446X_CMD_READY_FOR_BURST	0x30
#define SI446X_CMD_CHIP_RESET		0x40
#define SI446X_CMD_SET_CHANNEL		0x50

#define GPIO_CMD_GET_STATUE			0x10
#define GPIO_CMD_SET_485			0x20
#define GPIO_CMD_SET_LTE			0x30
#define GPIO_CMD_SET_LED1			0x40
#define GPIO_CMD_SET_LED2			0x50

struct modbus_rtu_pack
{
	unsigned char addr	:8;
	unsigned char pdu[1000];
};
struct fmb_tty
{
	bool_t running;
	bool_t tty_busy;
	bool_t radio_busy;
	int tty_fd;
	int gpio_fd;

	int radio_fd;
	int total_tx_packets;
	int total_tx_bytes;
	int total_rx_packets;
	int total_rx_bytes;
	uint8_t rxbuf[RADIO_MAX_PACKET];
	uint8_t *rx_wptr;
	uint16_t rxlen;
	pthread_t thread;
	int (*cb)(void *tty, fmb_transport_type_e trans, int fd, void *data, size_t len, bool_t is_broadcast, void *user_data);
	void *user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

	fmb_tty_t *fmb_tty_init();
	int fmb_tty_uninit(fmb_tty_t *tty);

	int fmb_tty_send(fmb_tty_t *tty, fmb_transport_type_e type, void *data, size_t len, bool_t is_rsp);
	int fmb_tty_set_recved_cb(fmb_tty_t *tty, int (*cb)(void *tty, fmb_transport_type_e trans, int fd, void *data, size_t len, bool_t is_broadcast, void *user_data), void *user_data);

	int fmb_tty_set_speed(fmb_tty_t *tty, int speed);
	int fmb_tty_set_data(fmb_tty_t *tty, int data);
	int fmb_tty_set_parity(fmb_tty_t *tty, char parity);
	int fmb_tty_set_stop(fmb_tty_t *tty, int stop);
	int fmb_tty_set_chn(fmb_tty_t *tty, uint16_t chn);

	__suseconds_t fmb_tty_get_interval();
	bool_t fmb_tty_is_busy(fmb_tty_t *tty);

#ifdef __cplusplus
}
#endif
#endif