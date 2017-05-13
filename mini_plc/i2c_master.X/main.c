#include "mcc_generated_files/mcc.h"
#include "protocol.h"
#include "i2c1_util.h"
#include <stdlib.h>
#include <string.h>

#define _XTAL_FREQ 500000
#define DEVICE_ID "BACKPLANE_MASTER"

// Backplane-master-specific commands
#define I2C "I2C"
#define MAP "MAP"
#define STS "STS"

// Debug commands
#define DEV "DEV"
#define REG "REG"
#define RED "RED"
#define WRT "WRT"
#define SEN "SEN"

#define MAX_DEV_ADDR 48  // it must be larger than 16 and multipels of 8
#define MASK 0x01

#define S_PLG 0x00
#define S_ADT 0x01
#define S_INV 0x02
#define S_SEN 0x03

const uint8_t MAX_Y = MAX_DEV_ADDR/8;

bool running = true;
uint8_t timer_cnt = 0;
bool do_func = false;
uint8_t read_buf[16];
uint8_t dev_map[MAX_Y];  // I2C slave device map

uint8_t dev_map_iterator(void);

void start_handler(void) {
    uint8_t dev_addr, status;
    for (dev_addr = dev_map_iterator(); dev_addr > 0 ; dev_addr = dev_map_iterator()) {
        if (i2c1_write_no_data(dev_addr, STA_I2C) > 0) {
            printf("!:%%%d:START FAILED\n", dev_addr);
        }        
    }
    running = true;
}

void stop_handler(void) {
    uint8_t dev_addr, status;
    for (dev_addr = dev_map_iterator(); dev_addr > 0 ; dev_addr = dev_map_iterator()) {
        if (i2c1_write_no_data(dev_addr, STP_I2C) > 0) {
            printf("!:%%%d:STOP FAILED\n", dev_addr);
        }        
    }
    running = false;
}

void set_handler(uint8_t value) {
}

uint8_t schedule[4] = {S_PLG, S_ADT, S_INV, S_SEN};
uint8_t current[2];
        
void tmr0_handler(void) {
    static uint8_t position = 0;
    if (++timer_cnt > 50 && !do_func) {  // 50msec time slot
        timer_cnt = 0;
        switch(schedule[position++]) {
            case S_PLG:
                current[0] = S_PLG;
                break;
            case S_ADT:
                current[0] = S_ADT;
                break;
            case S_INV:
                current[0] = S_INV;
                current[1] = 0x12;
                break;
            case S_SEN:
                current[0] = S_SEN;
                current[1] = 0x12;
                break;
        }
        if (position > 3) position = 0;
        do_func = true;
    }
 }

void clear_dev_map(void) {
    uint8_t y;
    for (y=0; y<MAX_Y; y++) {
        dev_map[y] = 0;
    }
}

uint8_t numbers_of_dev(void) {
    uint8_t num = 0;
    uint8_t x, y;
    for (y=0; y<MAX_Y; y++) {
        for (x=0; x<8; x++) {
            if ((dev_map[y] & (0b00000001 << x)) > 0) ++num;
        }
    }  
    return num;
}

void add_dev(uint8_t dev_addr) {
    uint8_t x, y;
    if (dev_addr >= 1 && dev_addr <= MAX_DEV_ADDR) {
        y = dev_addr / 8;
        x = dev_addr % 8;
        dev_map[y] = dev_map[y] | (0x01 << x);
    }
}

void del_dev(uint8_t dev_addr) {
    uint8_t x, y;
    if (dev_addr >= 1 && dev_addr <= MAX_DEV_ADDR) {
        y = dev_addr / 8;
        x = dev_addr % 8;
        dev_map[y] = dev_map[y] & ~(0x01 << x);
    }
}

void print_dev_map(void) {
    uint8_t i;
    uint8_t len = numbers_of_dev();
    if (len > 0) {
        len--;
        printf("$:MAP:");
        for (i=0; i<len; i++) printf("%d,", dev_map_iterator());
        printf("%d\n", dev_map_iterator());
    } else {
        printf("!:NO SLAVE FOUND\n");
    }
}

/**
 * @fn device map iterator
 * @return device address
 */
uint8_t dev_map_iterator() {
    static uint8_t xx = 0;
    static uint8_t yy = 0;
    static bool start = true;
    bool exist = false;
    uint8_t dev_addr;
    uint8_t test;
    
    do {
        if (xx > 7) {
            xx = 0;
            ++yy;
        }
        if (yy >= MAX_Y) {
            xx = 0;
            yy = 0;
            break;
        }
        test = (MASK << xx) & dev_map[yy];
        if (test) {
            dev_addr = yy * 8 + xx;
            exist = true;
        }
        ++xx;
    } while (!exist);
    
    if (exist) {
        exist = false;
        return dev_addr;
    } else {
        return 0;
    }
}

uint8_t sen(uint8_t dev_addr) {
    uint8_t status;
    uint8_t type;
    uint8_t length ,data, i;
    status = i2c1_read(dev_addr, SEN_I2C, &type, 1);
    printf("...%d\n", type);
    if (status == 0 && type != TYPE_NO_DATA) {
        status = i2c1_read(dev_addr, SEN_I2C, &length, 1);
        if (status == 0) {
            status = i2c1_read(dev_addr, SEN_I2C, &read_buf[0], length);
            if (status == 0) {
                PROTOCOL_Print_TLV(dev_addr, type, length, &read_buf[0]);
            }
        }                    
    }   
    return status;
}

void scan_dev(void) {
    uint8_t dev_addr, status;
    for (dev_addr=1; dev_addr<=MAX_DEV_ADDR; dev_addr++) {
        status = i2c1_read(dev_addr, WHO_I2C, &read_buf[0], 1);
        // printf("%d %d\n", dev_addr, status);
        if (status == 0) {
            add_dev(read_buf[0]);
        }
    }  
}

void loop_func(void) {
    uint8_t dev_addr;
    uint8_t status;
    if (do_func) {
        switch(current[0]) {
            case S_PLG:           
                status = i2c1_write_no_data(GENERAL_CALL_ADDRESS, PLG_I2C);
                if (status == 0) scan_dev();
                break;
            case S_ADT:
                break;
            case S_INV:
                dev_addr = current[1];
                i2c1_write_no_data(dev_addr, INV_I2C);
                break;
            case S_SEN:
                dev_addr = current[1];
                status = sen(dev_addr);
                if (status > 0) del_dev(dev_addr);
                break;
        }
        do_func = false;
    }
}

void extension_handler(uint8_t *buf) {
    uint8_t data;
    uint8_t type;
    uint8_t length;
    uint8_t i, status;

    /***** For debug commands *****/
    static uint8_t dev_addr;
    static uint8_t reg_addr;

    if (!strncmp(I2C, buf, 3)) {
        BACKPLANE_SLAVE_ADDRESS = atoi(&buf[4]);
    } else if (!strncmp(WHO, buf, 3)) {
        status = i2c1_read(BACKPLANE_SLAVE_ADDRESS, WHO_I2C, &data, 1);
        if (status == 0) printf("$:WHO:%d\n", data);
        else printf("!\n");
    } else if (!strncmp(MAP, buf, 3)) {
        print_dev_map();
    } else if (!strncmp(SAV, buf, 3)) {
        status = i2c1_write_no_data(BACKPLANE_SLAVE_ADDRESS, SAV_I2C);
    } else if (!strncmp(STA, buf, 3)) {
        status = i2c1_write_no_data(BACKPLANE_SLAVE_ADDRESS, STA_I2C);
    } else if (!strncmp(STP, buf, 3)) {
        status = i2c1_write_no_data(BACKPLANE_SLAVE_ADDRESS, STP_I2C);
        if (status == 0) printf("*:STP:ACK\n");
        else printf("!:STP:NACK\n");
    } else if (!strncmp(SET, buf, 3)) {
        data = atoi(&buf[4]);
        i2c1_write_no_data(BACKPLANE_SLAVE_ADDRESS, SET_I2C);
        i2c1_write_no_data(BACKPLANE_SLAVE_ADDRESS, data);        
    } else if (!strncmp(GET, buf, 3)) {
        i2c1_read(BACKPLANE_SLAVE_ADDRESS, GET_I2C, &data, 1);
        printf("$:GET:%d\n", data);
    /***** Debug commands *****/
    } else if (!strncmp(DEV, buf, 3)) {
        dev_addr = atoi(&buf[4]);
    } else if (!strncmp(REG, buf, 3)) {
        reg_addr = atoi(&buf[4]);
    } else if (!strncmp(RED, buf, 3)) {
        i2c1_read(dev_addr, reg_addr, &data, 1);
        printf("%d\n", data);
    } else if (!strncmp(WRT, buf, 3)) {
        data = atoi(&buf[4]);
        i2c1_write_no_data(dev_addr, reg_addr);
        i2c1_write_no_data(dev_addr, data);
    } else if (!strncmp(SEN, buf, 3)) {
        if (sen(BACKPLANE_SLAVE_ADDRESS) > 0) {
            printf("!:SEN:DATA NOT READY\n");
        }
    /* Extended commands */
    } else {
        length = 0;
        do {
        } while (buf[length++] != '\0');
        // printf("Extended command, length: %s, %d\n", buf, length);
        i2c1_write_no_data(BACKPLANE_SLAVE_ADDRESS, EXT_I2C);
        i2c1_write_no_data(BACKPLANE_SLAVE_ADDRESS, length);
        
        for (i=0; i<length; i++) {
            // printf("%c", buf[i]);
            status = i2c1_write_no_data(BACKPLANE_SLAVE_ADDRESS, (uint8_t)buf[i]);                 
        }
        // printf("\n");
    }
}

void main(void)
{
    clear_dev_map();

    SYSTEM_Initialize();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    TMR0_Initialize();
    TMR0_SetInterruptHandler(tmr0_handler);

    I2C1_Initialize();
    
    EUSART_Initialize();

    PROTOCOL_Initialize(DEVICE_ID, start_handler, stop_handler, set_handler);
    PROTOCOL_Set_Func(loop_func);
    PROTOCOL_Set_Extension_Handler(extension_handler);
    
    scan_dev();
    print_dev_map();

    PROTOCOL_Loop();
}