void lcd_init(void);
void lcd_goto(uint8_t row, uint8_t col);
void lcd_puts(const char *s);
void lcd_putc(char);
void lcd_delay_us(volatile uint32_t);
