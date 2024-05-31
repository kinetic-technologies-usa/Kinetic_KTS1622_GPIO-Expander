/**
 * @file test_pullup.c
 * @brief A program to configure a GPIO line with pull-up resistor disabled using libgpiod.
 * 
 * This program opens a GPIO chip, requests a GPIO line as an input with pull-up resistor disabled,
 * and then releases the line.
 * 
 * @details
 * - Uses the libgpiod library to interface with GPIO lines.
 * - Specifies the device using `gpiod_chip_open_by_name("gpiochip2")`.
 * - The GPIO chip and line details can be verified using the `gpiodetect` command.
 * 
 * Compilation command:
 * @code
 * gcc test_pullup.c -o test_pullup -lgpiod
 * @endcode
 * 
 * Usage:
 * @code
 * ./test_pullup
 * @endcode
 * 
 * @author
 * Koji Asakawa
 * 
 * @date
 * 5/31/2024
 */

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

// Specifying the device: gpiod_chip_open_by_name("gpiochip2"), gpiochip2 is given in 'gpiodetect'.
// $ gcc test_pullup.c -o test_pullup -lgpiod


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
    line = gpiod_chip_get_line(chip, 6);
    if (!line) {
        perror("Get line failed");
        gpiod_chip_close(chip);
        return -1;
    }
    // Request line as input with pull-up
    ret = gpiod_line_set_flags(line, GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE);
    if (ret < 0) {
        perror("Request line as input with pull-up failed");
	    printf("ret: %d\n", ret);
        gpiod_chip_close(chip);
        return -1;
    }

    // Your code to interact with the line

    // Release line and close chip
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}

