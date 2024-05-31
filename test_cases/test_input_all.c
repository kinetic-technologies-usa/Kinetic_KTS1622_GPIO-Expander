/**
 * @file test_input_pullup.c
 * @brief A program to read GPIO pin values using the libgpiod library.
 *
 * This program demonstrates how to read the state of multiple GPIO pins configured
 * as inputs. It uses the libgpiod library to interact with the GPIO subsystem.
 *
 * The program performs the following steps:
 * 1. Opens the GPIO chip specified by "gpiochip2".
 * 2. Configures a set number of GPIO lines (pins) as inputs.
 * 3. Reads the state of the configured GPIO lines in a loop, printing their values.
 * 4. Releases the GPIO lines and closes the GPIO chip before exiting.
 *
 * To compile the program:
 * @code
 * $ gcc test_input_pullup.c -o test_input_pullup -lgpiod
 * @endcode
 *
 * Usage:
 * @code
 * $ ./test_input_pullup
 * @endcode
 * @note This program requires the libgpiod library.
 * @see https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/
 * @author Koji Asakawa
 * @date 5/13/2024
 */

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

// Specifying the device: gpiod_chip_open_by_name("gpiochip2"), gpiochip2 is given in 'gpiodetect'.
// $ gcc test_input_pullup.c -o test_input_pullup -lgpiod

#define MAX_TEST_PIN_COUNT  16
#define MAX_TEST_COUNT      200

int main(int argc, char **argv) {
    struct gpiod_chip *chip;
    struct gpiod_line *line[MAX_TEST_PIN_COUNT];         // A0 to A7 pins
    int ret;

    // Open the GPIO chip.
    chip = gpiod_chip_open_by_name("gpiochip2");
    if (!chip) {
        perror("Open chip failed");
        return -1;
    }

    // Get the GPIO line
    for(int i=0;i<MAX_TEST_PIN_COUNT;i++){
        line[i] = gpiod_chip_get_line(chip, i);
        if (!line[i]) {
            perror("Get line failed");
            gpiod_chip_close(chip);
            return -1;
        }
        if (gpiod_line_request_input(line[i], "example_app") < 0) {
            perror("Request line as input failed");
            gpiod_line_release(line[i]);
            gpiod_chip_close(chip);
            return -1;
        }
    }

    // Read the line state
    for (int i = 0; i < MAX_TEST_COUNT; i++) {
        for(int i=0;i<MAX_TEST_PIN_COUNT;i++){
            int value = gpiod_line_get_value(line[i]);
            printf("[%d]%d ", i, value);
        }
        printf("\n");
        sleep(1);       // Sleep for 1 second
    }

    // Release line and close chip
    for(int i=0;i<MAX_TEST_PIN_COUNT;i++){
        gpiod_line_release(line[i]);
    }
    gpiod_chip_close(chip);

    return 0;
}
