#include "fsl_gpio.h"
#include "pin_mux.h"

int main(void) {
    BOARD_InitBootPins();

    while (1) {
        // Poll switch and toggle LED
        if (GPIO_PinRead(BOARD_BT_E_GPIO, BOARD_BT_E_PIN) == 0) {
            GPIO_PinWrite(BOARD_LED_B_GPIO, BOARD_LED_B_PIN, 1);
        }
    }
}
