#include "lcd_driver.h"
#include "common.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "peripherals.h"
#include <stdio.h>
#include <sys/lock.h>
#include <sys/param.h>
#include <unistd.h>

#ifndef USE_LVGL_DISPLAY
#include "font.h"

static const char *TAG = "lcd_driver";
/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; // No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

typedef struct
{
    uint16_t width;       // LCD 宽度
    uint16_t height;      // LCD 高度
    uint16_t id;          // LCD ID
    uint8_t dir;          // 横屏还是竖屏控制：0，竖屏；1，横屏
    uint16_t back_color;  // 背景色
    uint16_t point_color; // 画笔颜色
} lcd_dev_t;

static lcd_dev_t g_lcd_info = {240, 320, 0x9341, 0, WHITE, BLUE}; 
uint16_t *g_lines;
spi_device_handle_t g_spi;

DRAM_ATTR static const lcd_init_cmd_t ili_init_cmds[] = {
    /* Power control B, power control = 0, DC_ENA = 1 */
    {0xCF, {0x00, 0x83, 0X30}, 3},
    /* Power on sequence control,
     * cp1 keeps 1 frame, 1st frame enable
     * vcl = 0, ddvdh=3, vgh=1, vgl=2
     * DDVDH_ENH=1
     */
    {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
    /* Driver timing control A,
     * non-overlap=default +1
     * EQ=default - 1, CR=default
     * pre-charge=default - 1
     */
    {0xE8, {0x85, 0x01, 0x79}, 3},
    /* Power control A, Vcore=1.6V, DDVDH=5.6V */
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
    /* Pump ratio control, DDVDH=2xVCl */
    {0xF7, {0x20}, 1},
    /* Driver timing control, all=0 unit */
    {0xEA, {0x00, 0x00}, 2},
    /* Power control 1, GVDD=4.75V */
    {0xC0, {0x26}, 1},
    /* Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3 */
    {0xC1, {0x11}, 1},
    /* VCOM control 1, VCOMH=4.025V, VCOML=-0.950V */
    {0xC5, {0x35, 0x3E}, 2},
    /* VCOM control 2, VCOMH=VMH-2, VCOML=VML-2 */
    {0xC7, {0xBE}, 1},
    /* Memory access control, MX=MY=0, MV=1, ML=0, BGR=1, MH=0 */
    {0x36, {0x48}, 1}, // 扫描方向
    /* Pixel format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Frame rate control, f=fosc, 70Hz fps */
    {0xB1, {0x00, 0x1B}, 2},
    /* Enable 3G, disabled */
    {0xF2, {0x08}, 1},
    /* Gamma set, curve 1 */
    {0x26, {0x01}, 1},
    /* Positive gamma correction */
    {0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
    /* Negative gamma correction */
    {0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
    /* Column address set, SC=0, EC=0xEF */
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    /* Page address set, SP=0, EP=0x013F */
    {0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
    /* Memory write */
    {0x2C, {0}, 0},
    /* Entry mode set, Low vol detect disabled, normal display */
    {0xB7, {0x07}, 1},
    /* Display function control */
    {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
    /* Sleep out */
    {0x11, {0}, 0x80},
    /* Display on */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); // Zero out the transaction
    t.length    = 8;          // Command is 8 bits
    t.tx_buffer = &cmd;       // The data is the cmd itself
    t.user      = (void *)0;  // D/C needs to be set to 0
    if (keep_cs_active) {
        t.flags = SPI_TRANS_CS_KEEP_ACTIVE; // Keep CS active after data transfer
    }
    ret = spi_device_polling_transmit(spi, &t); // Transmit!
    assert(ret == ESP_OK);                      // Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) {
        return; // no need to send anything
    }
    memset(&t, 0, sizeof(t));                           // Zero out the transaction
    t.length    = len * 8;                              // Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;                                 // Data
    t.user      = (void *)1;                            // D/C needs to be set to 1
    ret         = spi_device_polling_transmit(spi, &t); // Transmit!
    assert(ret == ESP_OK);                              // Should have had no issues.
}

// This function is called (in irq context!) just before a transmission starts. It will
// set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(EXAMPLE_PIN_NUM_LCD_DC, dc);
}

uint32_t lcd_get_id(spi_device_handle_t spi)
{
    // When using SPI_TRANS_CS_KEEP_ACTIVE, bus must be locked/acquired
    spi_device_acquire_bus(spi, portMAX_DELAY);

    // get_id cmd
    lcd_cmd(spi, 0x04, true);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * 3;
    t.flags  = SPI_TRANS_USE_RXDATA;
    t.user   = (void *)1;

    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);

    // Release bus
    spi_device_release_bus(spi);

    return *(uint32_t *)t.rx_data;
}

// Initialize the display
void lcd_init(spi_device_handle_t spi)
{
    int cmd = 0;
    const lcd_init_cmd_t *lcd_init_cmds;

    // Initialize non-SPI GPIOs
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask  = ((1ULL << EXAMPLE_PIN_NUM_LCD_DC) | (1ULL << EXAMPLE_PIN_NUM_BK_LIGHT));
    io_conf.mode          = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en    = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    // detect LCD type
    uint32_t lcd_id = lcd_get_id(spi);
    int lcd_type;

    printf("LCD ID: %08" PRIx32 "\n", lcd_id);
    if (lcd_id == 0) {
        // zero, ili
        printf("ILI9341 detected.\n");
    } else {
        // none-zero, ST
        printf("ST7789V detected.\n");
    }

    printf("LCD ILI9341 initialization.\n");
    lcd_init_cmds = ili_init_cmds;

    // Send all the commands
    while (lcd_init_cmds[cmd].databytes != 0xff) {
        lcd_cmd(spi, lcd_init_cmds[cmd].cmd, false);
        lcd_data(spi, lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes & 0x1F);
        if (lcd_init_cmds[cmd].databytes & 0x80) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        cmd++;
    }

    // Enable backlight
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);
}

/* To send a set of lines we have to send a command, 2 data bytes, another command, 2 more data bytes and another command
 * before sending the line data itself; a total of 6 transactions. (We can't put all of this in just one transaction
 * because the D/C line needs to be toggled in the middle.)
 * This routine queues these commands up as interrupt transactions so they get
 * sent faster (compared to calling spi_device_transmit several times), and at
 * the mean while the lines for next transactions can get calculated.
 */
static void send_lines(spi_device_handle_t spi, int xpos, int ypos, int width, int height, uint16_t *linedata)
{
    esp_err_t ret;
    int x;
    // Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    // function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    // In theory, it's better to initialize trans and data only once and hang on to the initialized
    // variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            // Even transfers are commands
            trans[x].length = 8;
            trans[x].user   = (void *)0;
        } else {
            // Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user   = (void *)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;                       // Column Address Set
    trans[1].tx_data[0] = xpos >> 8;                  // Start Col High
    trans[1].tx_data[1] = xpos & 0xff;                // Start Col Low
    trans[1].tx_data[2] = (xpos + width - 1) >> 8;    // End Col High
    trans[1].tx_data[3] = (xpos + width - 1) & 0xff;  // End Col Low
    trans[2].tx_data[0] = 0x2B;                       // Page address set
    trans[3].tx_data[0] = ypos >> 8;                  // Start page high
    trans[3].tx_data[1] = ypos & 0xff;                // start page low
    trans[3].tx_data[2] = (ypos + height - 1) >> 8;   // end page high
    trans[3].tx_data[3] = (ypos + height - 1) & 0xff; // end page low
    trans[4].tx_data[0] = 0x2C;                       // memory write
    trans[5].tx_buffer  = linedata;                   // finally send the line data
    trans[5].length     = width * 2 * 8 * height;     // Data length, in bits
#if CONFIG_LCD_BUFFER_IN_PSRAM
    trans[5].flags = SPI_TRANS_DMA_USE_PSRAM; // using PSRAM
#else
    trans[5].flags = 0; // undo SPI_TRANS_USE_TXDATA flag
#endif

    // Queue all transactions.
    for (x = 0; x < 6; x++) {
        ret = spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
        assert(ret == ESP_OK);
    }
}

static void send_line_finish(spi_device_handle_t spi)
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    // Wait for all 6 transactions to be done and get back the results.
    for (int x = 0; x < 6; x++) {
        ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret == ESP_OK);
    }
}

static void display_pretty_colors(spi_device_handle_t spi, uint16_t color)
{
    for (int i = 0; i < g_lcd_info.width * PARALLEL_LINES; i++) {
        g_lines[i] = color; // 填充颜色
    }
    for (int y = 0; y < g_lcd_info.height; y += PARALLEL_LINES) {
        send_lines(spi, 0, y, g_lcd_info.width, PARALLEL_LINES, g_lines);
        send_line_finish(spi);
    }
}

void LCD_ShowChar(spi_device_handle_t spi, uint16_t x, uint16_t y, uint8_t num, uint8_t size, uint8_t mode)
{
    uint8_t temp       = 0, t1, t, xwidth;
    uint16_t x0        = x;
    uint16_t y0        = y;
    uint16_t colortemp = g_lcd_info.point_color;
    xwidth             = size / 2;
    num                = num - ' '; // 得到偏移后的值
    if (!mode) {                    // 非叠加方式
        for (t = 0; t < size; t++) {
            if (size == 12) {
                temp = asc2_1206[num][t]; // 调用1206字体
            } else {
                temp = asc2_1608[num][t]; // 调用1608字体
            }
            for (t1 = 0; t1 < 8; t1++) {
                if (temp & 0x01) {
                    g_lcd_info.point_color = colortemp;
                } else {
                    g_lcd_info.point_color = g_lcd_info.back_color;
                }
                g_lines[t * xwidth + t1] = g_lcd_info.point_color; // 先存起来再显示
                temp >>= 1;
                y++;
                if (y >= g_lcd_info.height) {
                    g_lcd_info.point_color = colortemp;
                    return;
                } // 超区域了
                if ((y - y0) == size) {
                    y = y0;
                    x++;
                    if (x >= g_lcd_info.width) {
                        g_lcd_info.point_color = colortemp;
                        return;
                    } // 超区域了
                    break;
                }
            }
        }
    } else { // 叠加方式
        for (t = 0; t < size; t++) {
            if (size == 12) {
                temp = asc2_1206[num][t]; // 调用1206字体
            } else {
                temp = asc2_1608[num][t]; // 调用1608字体
            }
            for (t1 = 0; t1 < 8; t1++) {
                if (temp & 0x01) {                                        // 从左到右 逐列扫描
                    g_lines[t * xwidth + t1] = g_lcd_info.point_color; // 先存起来再显示
                } else {
                    g_lines[t * xwidth + t1] = g_lcd_info.back_color;
                }

                temp >>= 1;
                y++;
                if (y >= g_lcd_info.height) {
                    g_lcd_info.point_color = colortemp;
                    return;
                } // 超区域了
                if ((y - y0) == size) {
                    y = y0;
                    x++;
                    if (x >= g_lcd_info.width) {
                        g_lcd_info.point_color = colortemp;
                        return;
                    } // 超区域了
                    break;
                }
            }
        }
    }
    send_lines(spi, x0, y0, xwidth, size, g_lines);
    send_line_finish(spi);
    g_lcd_info.point_color = colortemp;
}

void ShowString(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p)
{
    uint16_t x0 = x;
    width += x;
    height += y;
    while ((*p <= '~') && (*p >= ' ')) { // 判断是不是非法字符!
        if (x >= width) {
            x = x0;
            y += size;
        }
        if (y >= height) {
            break; // 退出
        }
        LCD_ShowChar(g_spi, x, y, *p, size, 0);
        x += size / 2;
        p++;
    }
}

void InitLCD(void)
{
    ESP_LOGI(TAG, "LVGL is not enabled");
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num     = EXAMPLE_PIN_NUM_MISO,
        .mosi_io_num     = EXAMPLE_PIN_NUM_MOSI,
        .sclk_io_num     = EXAMPLE_PIN_NUM_SCLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = PARALLEL_LINES * g_lcd_info.width * 2 + 8};
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 26 * 1000 * 1000,              // Clock out at 26 MHz
        .mode           = 0,                             // SPI mode 0
        .spics_io_num   = EXAMPLE_PIN_NUM_LCD_CS,        // CS pin
        .queue_size     = 7,                             // We want to be able to queue 7 transactions at a time
        .pre_cb         = lcd_spi_pre_transfer_callback, // Specify pre-transfer callback to handle D/C line
    };
    // Initialize the SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    // Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &g_spi);
    ESP_ERROR_CHECK(ret);
    // Initialize the LCD
    lcd_init(g_spi);

#if CONFIG_LCD_BUFFER_IN_PSRAM
    uint32_t mem_cap = MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA;
    printf("Get LCD buffer from PSRAM\n");
#else
    uint32_t mem_cap = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
    printf("Get LCD buffer from internal\n");
#endif

    // Allocate memory for the pixel buffers
    g_lines = spi_bus_dma_memory_alloc(LCD_HOST, g_lcd_info.width * PARALLEL_LINES * sizeof(uint16_t), mem_cap);
    display_pretty_colors(g_spi, WHITE);

    InitAll();

    // free(g_lines);
}

#endif
