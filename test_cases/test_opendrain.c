/**
 * @file test_opendrain.c
 * @brief A simple program to demonstrate the use of libgpiod for controlling GPIO lines in open-drain mode.
 * 
 * This program opens a GPIO chip, requests a GPIO line as an output in open-drain mode, and then releases the line.
 * 
 * @details
 * - Uses the libgpiod library to interface with GPIO lines.
 * - Specifies the device using `gpiod_chip_open_by_name("gpiochip2")`.
 * - The GPIO chip and line details can be verified using the `gpiodetect` command.
 * 
 * Compilation command:
 * @code
 * gcc test_opendrain.c -o test_opendrain -lgpiod
 * @endcode
 * Usage:
 * @code
 * ./test_opendrain
 * @endcode
 * @author
 * Koji Asakawa
 * @date
 * 5/31/2024
 */

#include <gpiod.h>
#include <stdio.h>

// Specifying the device: gpiod_chip_open_by_name("gpiochip2"), gpiochip2 is given in 'gpiodetect'.
// $ gcc test_opendrain.c -o test_opendrain -lgpiod

int main(int argc, char **argv) {
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    bool value = true;
    int ret;

    // Open the GPIO chip
    chip = gpiod_chip_open_by_name("gpiochip2");
    if (!chip) {
        perror("Open chip failed");
        return -1;
    }

    // Get the GPIO line
    line = gpiod_chip_get_line(chip, 7);
    if (!line) {
        perror("Get line failed");
        gpiod_chip_close(chip);
        return -1;
    }
    // Request line as output open-drain mode
    ret = gpiod_line_request_output_flags(line, "my-gpio-program", GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN, 0);
    if (ret < 0) {
        perror("Request line as input with pull-up failed");
	    printf("ret: %d\n", ret);
        gpiod_chip_close(chip);
        return -1;
    }

    // Release line and close chip
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}

