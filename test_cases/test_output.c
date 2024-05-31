/**
 * @file test_output.c
 * @brief A simple program to demonstrate the use of libgpiod for controlling GPIO lines.
 * 
 * This program opens a GPIO chip, requests a GPIO line as an output, toggles the line state a few times,
 * and then releases the line. It also includes an example of requesting a line in open-drain mode.
 * 
 * @details
 * - Uses the libgpiod library to interface with GPIO lines.
 * - Specifies the device using `gpiod_chip_open_by_name("gpiochip2")`.
 * - The GPIO chip and line details can be verified using the `gpiodetect` command.
 * 
 * Compilation command:
 * @code
 * gcc test_output.c -o test_output -lgpiod
 * @endcode
 * Usage:
 * @code
 * ./test_output
 * @endcode
 * @author
 * Koji Asakawa
 * @date
 * 5/31/2024
 */

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

// Specifying the device: gpiod_chip_open_by_name("gpiochip2"), gpiochip2 is given in 'gpiodetect'.
// $ gcc test_output.c -o test_output -lgpiod

int main(int argc, char **argv) {
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    bool value = true;
    int ret;

    // Open the GPIO chip.
    chip = gpiod_chip_open_by_name("gpiochip2");
    if (!chip) {
        perror("Open chip failed");
        return -1;
    }

    // Get the GPIO line
    line = gpiod_chip_get_line(chip, 0);
    if (!line) {
        perror("Get line failed");
        gpiod_chip_close(chip);
        return -1;
    }
    // Request output mode for the line
    if (gpiod_line_request_output(line, "example_app", 0) < 0) {
        perror("Request line as output failed");
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return -1;
    }

    // Toggle the line state a few times
    for (int i = 0; i < 5; i++) {
        value = !value; // Toggle the state
        gpiod_line_set_value(line, value);
        sleep(1); // Sleep for 1 second
    }

#if 1
    // Get the GPIO line
    line = gpiod_chip_get_line(chip, 7);
    if (!line) {
        perror("Get line failed");
        gpiod_chip_close(chip);
        return -1;
    }
    // Request line as input with pull-up
    ret = gpiod_line_request_output_flags(line, "my-gpio-program", GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN, 0);
    if (ret < 0) {
        perror("Request line as input with pull-up failed");
	printf("ret: %d\n", ret);
        gpiod_chip_close(chip);
        return -1;
    }
#endif

    // Your code to interact with the line

    // Release line and close chip
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    return 0;
}

