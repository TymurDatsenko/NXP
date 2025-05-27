#include "pin_mux.h"
#include "board.h"
#include "MK64F12.h"

/*— Macros for convenience —*/
#define LCD_GPIO(gpio, pin)    ((gpio)->PDDR |= (1U << (pin)))
#define LCD_SET(gpio, pin)     ((gpio)->PSOR  = (1U << (pin)))
#define LCD_CLR(gpio, pin)     ((gpio)->PCOR  = (1U << (pin)))

/*— Pin aliases —*/
#define LCD_RS_GPIO    BOARD_D_RS_GPIO
#define LCD_RS_PIN     BOARD_D_RS_PIN
#define LCD_E_GPIO     BOARD_D_E_GPIO
#define LCD_E_PIN      BOARD_D_E_PIN
#define LCD_D7_GPIO    BOARD_DB7_GPIO
#define LCD_D7_PIN     BOARD_DB7_PIN
#define LCD_D6_GPIO    BOARD_DB6_GPIO
#define LCD_D6_PIN     BOARD_DB6_PIN
#define LCD_D5_GPIO    BOARD_DB5_GPIO
#define LCD_D5_PIN     BOARD_DB5_PIN
#define LCD_D4_GPIO    BOARD_DB4_GPIO
#define LCD_D4_PIN     BOARD_DB4_PIN

/*— Simple busy-wait micro delays (~1μs per loop) —*/
static void lcd_delay_us(volatile uint32_t us)
{
    while (us--) {
        for (volatile int i = 0; i < 10; i++) { __NOP(); }
    }
}

/*— GPIO init: assume pinmux & port clocks already enabled by BOARD_InitPins() —*/
static void lcd_gpio_init(void)
{
    /* Just set all LCD pins to output */
    LCD_GPIO(LCD_RS_GPIO, LCD_RS_PIN);
    LCD_GPIO(LCD_E_GPIO,  LCD_E_PIN);
    LCD_GPIO(LCD_D7_GPIO, LCD_D7_PIN);
    LCD_GPIO(LCD_D6_GPIO, LCD_D6_PIN);
    LCD_GPIO(LCD_D5_GPIO, LCD_D5_PIN);
    LCD_GPIO(LCD_D4_GPIO, LCD_D4_PIN);
}

/*— Toggle the E line to latch data —*/
static void lcd_pulse_enable(void)
{
    LCD_SET(LCD_E_GPIO, LCD_E_PIN);
    lcd_delay_us(1);
    LCD_CLR(LCD_E_GPIO, LCD_E_PIN);
    lcd_delay_us(100);
}

/*— Send a 4-bit nibble to D4–D7 —*/
static void lcd_write_nibble(uint8_t nibble)
{
    /* put RS low or high already set by caller */
    /* clear data pins */
    LCD_CLR(LCD_D4_GPIO, LCD_D4_PIN);
    LCD_CLR(LCD_D5_GPIO, LCD_D5_PIN);
    LCD_CLR(LCD_D6_GPIO, LCD_D6_PIN);
    LCD_CLR(LCD_D7_GPIO, LCD_D7_PIN);
    /* set bits */
    if (nibble & 0x1) LCD_SET(LCD_D4_GPIO, LCD_D4_PIN);
    if (nibble & 0x2) LCD_SET(LCD_D5_GPIO, LCD_D5_PIN);
    if (nibble & 0x4) LCD_SET(LCD_D6_GPIO, LCD_D6_PIN);
    if (nibble & 0x8) LCD_SET(LCD_D7_GPIO, LCD_D7_PIN);
    lcd_pulse_enable();
}

/*— Send either a command (isData=0) or data byte (isData=1) —*/
static void lcd_send(uint8_t value, bool isData)
{
    if (isData) LCD_SET(LCD_RS_GPIO, LCD_RS_PIN);
    else        LCD_CLR(LCD_RS_GPIO, LCD_RS_PIN);

    /* high nibble */
    lcd_write_nibble(value >> 4);
    /* low nibble */
    lcd_write_nibble(value & 0x0F);
}

/*— Initialize HD44780 in 4-bit mode —*/
void lcd_init(void)
{
    lcd_gpio_init();
    lcd_delay_us(40000);          // >40 ms after Vcc rises

    /* special reset sequence */
    LCD_CLR(LCD_RS_GPIO, LCD_RS_PIN);
    lcd_write_nibble(0x3);
    lcd_delay_us(5000);           // >4.1 ms
    lcd_write_nibble(0x3);
    lcd_delay_us(150);            // >100 µs
    lcd_write_nibble(0x3);
    lcd_write_nibble(0x2);        // set 4-bit mode

    /* function set: 2 lines, 5×8 dots */
    lcd_send(0x28, false);
    /* display off */
    lcd_send(0x08, false);
    /* clear */
    lcd_send(0x01, false);
    lcd_delay_us(2000);
    /* entry mode: inc, no shift */
    lcd_send(0x06, false);
    /* display on, cursor off, blink off */
    lcd_send(0x0C, false);
}

/*— High-level drawing —*/
void lcd_putc(char c)
{
    lcd_send((uint8_t)c, true);
}

void lcd_puts(const char *s)
{
    while (*s) {
        lcd_putc(*s++);
    }
}

void lcd_goto(uint8_t row, uint8_t col)
{
    static const uint8_t row_addr[] = { 0x00, 0x40, 0x14, 0x54 };
    uint8_t addr = row_addr[row] + col;
    lcd_send(0x80 | addr, false);
}

void lcd_clr() {
    lcd_command(0x01);  // Clear display
    lcd_delay_us(2);        // Clearing takes ~1.53ms, so we wait 2ms
}
