#include "f2f_modbus.h"
#include "main.h"

// leave un-implemented
void _fbs_log(fbs_log_level level, fbs_log_module module, const char *file, int line, char *fmt, ...)
{
    //这部分需要采用单片机的语言进行实现，实现后应当将日志输出到调试工具中

    //level代表了日志的级别
    //module代表日志的发生的模块
    //在linux上，这种...代表可变参数，使用下面的语句可以把...中的内容按照fmt输出到这个fd中
    #if 0
    int fd = 0;
    va_start(args, fmt);
    vfprintf(fd, fmt, args);
    va_end(args);
    #endif
    return;
}

static int read_str_from_flash(unsigned char *buffer, unsigned char size, unsigned int addr)
{
    if(size < 64 || !buffer)
        return -1;
    for (unsigned int i = 0; i<MAX_MODBUS_PARAM_LENS/8; i++)
        memcpy(buffer + i*8, (uint64_t *)address + i, 8);
    return 0;
}

const char *fbs_cfg_get(const char *key, fbs_cfg_type_e type)
{
    //这个函数用于在配置文件中获取相关的配置信息
    //type在单片机上没有什么意义
    //key是需要获得配置名称
    //return一个char*，外面的程序再根据需要进行转换
    unsigned char *buffer = malloc(MAX_MODBUS_PARAM_LENS);
    if(!buffer)
        return NULL;
    if (strcmp(key, "modbus:tty_dev") == 0)
    {
        return read_str_from_flash(buffer, 64, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_DEV_OFFSET) == 0 ? buffer : NULL;
    }
    else if(strcmp(key, "modbus:tty_speed") == 0)
    {
        return read_str_from_flash(buffer, 64, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_SPEED_OFFSET) == 0 ? buffer: NULL;
    }
    else if(strcmp(key, "modbus:tty_data") == 0)
    {
        return read_str_from_flash(buffer, 64, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_DATA_OFFSET) == 0 ? buffer: NULL;
    }
    else if(strcmp(key, "modbus:tty_parity") == 0)
    {
        return read_str_from_flash(buffer, 64, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_PARITY_OFFSET) == 0 ? buffer: NULL;
    }
    else if(strcmp(key, "modbus:tty_stop") == 0)
    {
        return read_str_from_flash(buffer, 64, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_STOP_OFFSET) == 0 ? buffer: NULL;
    }
    else if(strcmp(key, "modbus:tty_slaveid") == 0)
    {
        return read_str_from_flash(buffer, 64, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_SLAVEID_OFFSET) == 0 ? buffer: NULL;
    }
    return NULL;
}

// 假设字符串最长64个字节，占据8个double word
static int write_str_to_flash(unsigned char *str, unsigned int address)
{
    unsigned char buffer[MAX_MODBUS_PARAM_LENS] = {0};
    if(str && strlen(str) > MAX_MODBUS_PARAM_LENS - 1)
        return -1;
    strcpy(buffer, str);
    for (unsigned int i = 0; i<MAX_MODBUS_PARAM_LENS/8; i++, buffer += 8, address += 8)
    {
        if (HAL_OK != FLASH_Program_DoubleWord(*(unsigned int*)buffer, address))
            return -1;
    }
    return 0;
}

// 直接保存进flash
int fbs_cfg_set(const char *key, const char *value, fbs_cfg_type_e type)
{
    //要判断需要存储的key是不是符合要求的，如果找不到相应的key，应当返回FBS_EXCEED_PARAM
    //按照我原来的逻辑，set只是在内存中临时存储一下，掉电不会保存。如果想永久存储，应该先set再屌用fbs_cfg_save
    //这里你们酌情处理
    HAL_StatusTypeDef status = HAL_OK;
    if (!key || !value)
        return FBS_EXCEED_PARAM;
    if (strcmp(key, "modbus:tty_dev") == 0)
    {
        return write_str_to_flash(value, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_DEV_OFFSET) == 0 ? FBS_SUCC : FBS_EXCEED_PARAM;
    }
    else if(strcmp(key, "modbus:tty_speed") == 0)
    {
        return write_str_to_flash(value, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_SPEED_OFFSET) == 0 ? FBS_SUCC : FBS_EXCEED_PARAM;
    }
    else if(strcmp(key, "modbus:tty_data") == 0)
    {
        return write_str_to_flash(value, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_DATA_OFFSET) == 0 ? FBS_SUCC : FBS_EXCEED_PARAM;
    }
    else if(strcmp(key, "modbus:tty_parity") == 0)
    {
        return write_str_to_flash(value, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_PARITY_OFFSET) == 0 ? FBS_SUCC : FBS_EXCEED_PARAM;
    }
    else if(strcmp(key, "modbus:tty_stop") == 0)
    {
        return write_str_to_flash(value, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_STOP_OFFSET) == 0 ? FBS_SUCC : FBS_EXCEED_PARAM;
    }
    else if(strcmp(key, "modbus:tty_slaveid") == 0)
    {
        return write_str_to_flash(value, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_SLAVEID_OFFSET) == 0 ? FBS_SUCC : FBS_EXCEED_PARAM;
    }
    else
        return FBS_EXCEED_PARAM;

    return FBS_SUCC;
}

// 始终返回成功
int fbs_cfg_save(fbs_cfg_type_e type)
{
    return FBS_SUCC;
}

int fbs_rand()
{
    //获得一个随机数，必须在单片机实现
    uint32_t randomNumber = 0;
    if (HAL_RNG_GenerateRandomNumber(&hRNG, &randomNumber) == HAL_OK)
    {
        // 成功生成随机数，在这里可以使用randomNumber变量
    }
    else
    {
        // 生成随机数失败，进行相应的错误处理
    }
    return randomNumber;
}

uint32_t fbs_uid_get()
{
    //获得本设备的出厂唯一ID，必须实现
	unsigned char buffer[MAX_MODBUS_PARAM_LENS] = {0};
    if( 0 == read_str_from_flash(buffer, MAX_MODBUS_PARAM_LENS, MODBUS_REG_BASE_ADDR+MODBUS_REG_TTY_SLAVEID_OFFSET))
    {
        unsigned int val = strtok(buffer, NULL, 16);
        return val;
    }
    else // 如果读取失败了呢？ 
    {

    }
    return 0;
}

void fbs_time_sync_pts(uint64_t pts)
{
    //输入的pts是单位为us的当前时间戳，这个数据需要同步给单片机。
    //必须实现
}

int fbs_serial_open(const char *dev, int speed, int databits, char parity, int stopbits)
{
    //打开串口，务必实现
    //串口始终打开的，直接返回文件描述符
    return FD_MODBUS_USART;
}

// 返回值类型改成freeRTOS的线程句柄
osThreadId_t fbs_thread_create(void *(*func)(void *), void *arg, const char *name)
{
    //创建线程，务必实现
    return osThreadNew(func, NULL, arg);
}
float swap_float_endian(float data) 
{
    union {
        float f;
        unsigned char bytes[4];
    } src, dest;
    src.f = data;
 
    dest.bytes[0] = src.bytes[3];
    dest.bytes[1] = src.bytes[2];
    dest.bytes[2] = src.bytes[1];
    dest.bytes[3] = src.bytes[0];
 
    return dest.f;
}
uint64_t htonll(uint64_t val)
{
    return (((uint64_t) htonl(val)) << 32) + htonl(val >> 32);
}
uint64_t ntohll(uint64_t val)
{
    return (((uint64_t) ntohl(val)) << 32) + ntohl(val >> 32);
}