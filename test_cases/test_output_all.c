/**
 * @file test_output_all.c
 * @brief A program to control multiple GPIO lines using libgpiod.
 * 
 * This program opens a GPIO chip, requests multiple GPIO lines as outputs,
 * toggles the state of these lines multiple times, and then releases the lines.
 * 
 * @details
 * - Uses the libgpiod library to interface with GPIO lines.
 * - Specifies the device using `gpiod_chip_open_by_name("gpiochip2")`.
 * - The GPIO chip and line details can be verified using the `gpiodetect` command.
 * 
 * Compilation command:
 * @code
 * gcc test_output_all.c -o test_output_all -lgpiod
 * @endcode
 * 
 * Usage:
 * @code
 * ./test_output_all
 * @endcode
 * Constants:
 * - MAX_TEST_PIN_COUNT: The number of GPIO lines to control.
 * - MAX_TEST_COUNT: The number of times to toggle the GPIO lines.
 * @author
 * Koji Asakawa
 * @date
 * 5/31/2024
 */

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

// Specifying the device: gpiod_chip_open_by_name("gpiochip2"), gpiochip2 is given in 'gpiodetect'.
// $ gcc test_output_all.c -o test_output_all -lgpiod

#define MAX_TEST_PIN_COUNT  16
#define MAX_TEST_COUNT      20

int main(int argc, char **argv) {
    struct gpiod_chip *chip;
    struct gpiod_line *line[MAX_TEST_PIN_COUNT];         // A0 to A7 pins
    bool value = true;
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
        // Request output mode for the line
        if (gpiod_line_request_output(line[i], "example_app", 0) < 0) {
            perror("Request line as output failed");
            gpiod_line_release(line[i]);
            gpiod_chip_close(chip);
            return -1;
        }
    }

    // Toggle the line state a few times
    for (int i = 0; i < MAX_TEST_COUNT; i++) {
        value = !value; // Toggle the state
        for(int i=0;i<MAX_TEST_PIN_COUNT;i++){
            gpiod_line_set_value(line[i], value);
        }
        sleep(1); // Sleep for 1 second
    }

    // Release line and close chip
    for(int i=0;i<MAX_TEST_PIN_COUNT;i++){
        gpiod_line_release(line[i]);
    }
    gpiod_chip_close(chip);

    return 0;
}

