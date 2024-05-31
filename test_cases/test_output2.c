/**
 * @file test_output2.c
 * @brief A program to demonstrate GPIO control using libgpiod for multiple GPIO chips.
 * 
 * This program opens two GPIO chips, requests GPIO lines as outputs, reads the value of one line,
 * and then releases the lines and closes the chips.
 * 
 * @details
 * - Uses the libgpiod library to interface with GPIO lines.
 * - Specifies the devices using `gpiod_chip_open_by_name("gpiochip2")` and `gpiod_chip_open_by_name("gpiochip3")`.
 * - The GPIO chips and line details can be verified using the `gpiodetect` command.
 * 
 * Compilation command:
 * @code
 * gcc test_output2.c -o test_output2 -lgpiod
 * @endcode
 * 
 * Usage:
 * @code
 * ./test_output2
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
// $ gcc test_output2.c -o test_output2 -lgpiod

int main(int argc, char **argv)
{

    struct gpiod_chip       *chip1;
    struct gpiod_chip       *chip2;
    unsigned int            chip1_num_lines;
    unsigned int            chip2_num_lines;
    unsigned                line2_0_value=0;

    struct gpiod_line       *line1_0, *line2_0;
    int                     ret;

    // Open the GPIO chip.
    chip1 = gpiod_chip_open_by_name("gpiochip2");
    chip2 = gpiod_chip_open_by_name("gpiochip3");
    if (!chip1 || !chip2) {
        perror("Open gpiochip failed");
        return -1;
    }

    chip1_num_lines = gpiod_chip_num_lines(chip1);      // getting max number of pins.
    chip2_num_lines = gpiod_chip_num_lines(chip2);      // getting max number of pins.
    printf("gpiochip2: Number of GPIO lines: %u\n", chip1_num_lines);
    printf("gpiochip3: Number of GPIO lines: %u\n", chip2_num_lines);

    // Get the GPIO line
    line1_0 = gpiod_chip_get_line(chip1, 0);
    line2_0 = gpiod_chip_get_line(chip2, 0);
    if (!line1_0 || !line2_0) {
        perror("Get line failed");
        gpiod_chip_close(chip1);
        gpiod_chip_close(chip2);
        return -1;
    }

    // SETTING: Request input mode for the line2_0
/*
    if (gpiod_line_request_input(line2_0, "example_app") < 0) {
        perror("Request line as input failed");
        gpiod_line_release(line2_0);
        gpiod_chip_close(chip2);
        return -1;
    }
*/

    // SETTING: Request output mode for the line1_0
    if (gpiod_line_request_output(line2_0, "example_app", 1) < 0) {
        perror("Request line as output failed");
        gpiod_line_release(line2_0);
        gpiod_chip_close(chip2);
        return -1;
    }

    // SETTING: Request output mode for the line1_0
    if (gpiod_line_request_output(line1_0, "example_app", 1) < 0) {
        perror("Request line as output failed");
        gpiod_line_release(line1_0);
        gpiod_chip_close(chip1);
        return -1;
    }

    // READING: Reading the IO port value
    line2_0_value = gpiod_line_get_value(line2_0);
    if (line2_0_value < 0) {
        perror("Read line value failed");
    } else {
        printf("line2_0_value Line value: %d\n", line2_0_value);
    }

    // Release line and close chip
    gpiod_line_release(line1_0);
    gpiod_line_release(line2_0);
    gpiod_chip_close(chip1);
    gpiod_chip_close(chip2);

    printf("Done..\n");

    return 0;
}

