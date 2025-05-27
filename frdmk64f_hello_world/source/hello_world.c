#include "fsl_gpio.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "lcd.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#define MAX_LIVES 6
#define MAX_WORD_LEN 16

const char* secretWords[] = {"Apple", "Bus", "Car", "Plane"};
char userGuess[MAX_WORD_LEN + 1];     // What player builds
int currentLetterIndex = 0;           // A-Z
int cursorPos = 0;                    // Where player is editing
int lives = MAX_LIVES;
const char* selectedWord;
int wordLen = 0;
bool locked[MAX_WORD_LEN] = { false };

void delay_ms(volatile uint32_t ms) {
    while (ms--) {
        for (volatile uint32_t i = 0; i < 6000; i++) {
            __NOP();
        }
    }
}

void blink_red_led() {
    for (int i = 0; i < 2; i++) {
        GPIO_PinWrite(BOARD_LED_R_GPIO, BOARD_LED_R_PIN, 1);
        delay_ms(150);
        GPIO_PinWrite(BOARD_LED_R_GPIO, BOARD_LED_R_PIN, 0);
        delay_ms(150);
    }
}

void init_game() {
    srand(1234); // Use time(NULL) if available
    int r = rand() % (sizeof(secretWords) / sizeof(secretWords[0]));
    selectedWord = secretWords[r];
    wordLen = strlen(selectedWord);

    for (int i = 0; i < wordLen; i++) {
        userGuess[i] = '_';
        locked[i] = false;
    }
    userGuess[wordLen] = '\0';
}

void update_display(bool blink) {
    lcd_init();

    // Line 1: show guessed letters with optional blinking one
    lcd_goto(1, 1);
    for (int i = 0; i < wordLen; i++) {
        if (i == cursorPos && !locked[i]) {
            if (blink)
                lcd_putc(' ');  // blink effect
            else
                lcd_putc('A' + currentLetterIndex);
        } else {
            lcd_putc(userGuess[i]);
        }
        lcd_putc(' ');
    }

    // Line 2: show lives
    char buf[17];
    sprintf(buf, "Lives: %d", lives);
    lcd_goto(2, 1);
    lcd_puts(buf);
}

bool word_guessed() {
    for (int i = 0; i < wordLen; i++) {
        if (toupper(selectedWord[i]) != userGuess[i]) return false;
    }
    return true;
}

void lock_letter() {
    char guess = 'A' + currentLetterIndex;
    userGuess[cursorPos] = guess;
    locked[cursorPos] = true;

    if (toupper(selectedWord[cursorPos]) != guess) {
        lives--;
        blink_red_led();
    }
}

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);

    gpio_pin_config_t inCfg = {kGPIO_DigitalInput, 0};
    gpio_pin_config_t outCfg = {kGPIO_DigitalOutput, 0};

    GPIO_PinInit(BOARD_BT_W_GPIO, BOARD_BT_W_PIN, &inCfg);
    GPIO_PinInit(BOARD_BT_E_GPIO, BOARD_BT_E_PIN, &inCfg);
    GPIO_PinInit(BOARD_BT_S_GPIO, BOARD_BT_S_PIN, &inCfg);
    GPIO_PinInit(BOARD_ENC_SW_GPIO, BOARD_ENC_SW_PIN, &inCfg);
    GPIO_PinInit(BOARD_LED_R_GPIO, BOARD_LED_R_PIN, &outCfg);

    lcd_init();

    init_game();
    update_display(false);

    while (1) {
        init_game();
        update_display(false);

        while (lives > 0 && !word_guessed()) {
            static bool blink = false;
            update_display(blink);
            blink = !blink;
            delay_ms(250);

            if (GPIO_PinRead(BOARD_BT_W_GPIO, BOARD_BT_W_PIN) == 0) {
                do {
                    cursorPos = (cursorPos - 1 + wordLen) % wordLen;
                } while (locked[cursorPos]);
                update_display(false);
                delay_ms(200);
            } else if (GPIO_PinRead(BOARD_BT_E_GPIO, BOARD_BT_E_PIN) == 0) {
                do {
                    cursorPos = (cursorPos + 1) % wordLen;
                } while (locked[cursorPos]);
                update_display(false);
                delay_ms(200);
            } else if (GPIO_PinRead(BOARD_BT_S_GPIO, BOARD_BT_S_PIN) == 0) {
                currentLetterIndex = (currentLetterIndex + 1) % 26;
                update_display(false);
                delay_ms(200);
            } else if (GPIO_PinRead(BOARD_ENC_SW_GPIO, BOARD_ENC_SW_PIN) == 0) {
                lock_letter();
                update_display(false);

                bool moved = false;
                for (int i = 0; i < wordLen; i++) {
                    int next = (cursorPos + 1 + i) % wordLen;
                    if (!locked[next]) {
                        cursorPos = next;
                        moved = true;
                        break;
                    }
                }

                if (!moved) break;

                while (GPIO_PinRead(BOARD_ENC_SW_GPIO, BOARD_ENC_SW_PIN) == 0) {}
                delay_ms(150);
            }
        }

        lcd_init();
        lcd_goto(1, 1);
        if (word_guessed()) {
            lcd_puts("You Win!");
        } else {
            lcd_puts("Game Over");
        }

        lcd_goto(2, 1);
        lcd_puts("Word: ");
        lcd_puts(selectedWord);

        // 👇 Wait for SW5 to restart
        while (GPIO_PinRead(BOARD_BT_N_GPIO, BOARD_BT_N_PIN) != 0) {
            delay_ms(100);
        }

        while (GPIO_PinRead(BOARD_BT_N_GPIO, BOARD_BT_N_PIN) == 0) {}
        delay_ms(200);
    }
}

