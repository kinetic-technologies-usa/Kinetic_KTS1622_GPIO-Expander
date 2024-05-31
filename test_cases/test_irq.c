/**
 * @file test_irq.c
 * @brief A program to detect GPIO edge events using the libgpiod library.
 *
 * This program demonstrates how to detect rising, falling, and both edge events
 * on GPIO pins using the libgpiod library. It specifies a GPIO chip and sets up
 * event notifications for GPIO lines.
 *
 * The program performs the following steps:
 * 1. Opens the GPIO chip specified by "gpiochip2".
 * 2. Configures GPIO lines for event notifications.
 * 3. Waits for edge events on a specified GPIO line and prints the event type.
 * 4. Releases the GPIO lines and closes the GPIO chip before exiting.
 *
 * To compile the program:
 * @code
 * $ gcc test_irq.c -o test_irq -lgpiod
 * @endcode
 * Usage:
 * @code
 * $ ./test_irq
 * @endcode
 * @note This program requires the libgpiod library.
 * @see https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/
 * @author Koji Asakawa
 * @date 5/31/2024
 */
#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

// Specifying the device on GPIO_CHIP, gpiochip2 is given in 'gpiodetect'.
// $ gcc test_irq.c -o test_irq -lgpiod

#define CONSUMER                "Consumer"
#define GPIO_CHIP               "gpiochip2"
#define GPIO_CHIP_PIN_COUNT     16              // Pin number 0 to 15

#define GPIO_RISING_EDGE_PIN    0               // Pin number 0 to 15
#define GPIO_FALLING_EDGE_PIN   2               // Pin number 0 to 15
#define GPIO_BOTH_EDGES_PIN     4               // Pin number 0 to 15

int main(void)
{
    struct gpiod_chip           *chip;
    struct gpiod_line           *line[GPIO_CHIP_PIN_COUNT];
    struct gpiod_line_event     event;
    int                         ret;

    chip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!chip) {
        perror("Open chip failed");
        return 1;
    }

    // Get the GPIO lines
    for (int i = 0; i < GPIO_CHIP_PIN_COUNT; i++) {
        line[i] = gpiod_chip_get_line(chip, i);
        if (!line[i]) {
            perror("Get line failed");
            gpiod_chip_close(chip);
            return 1;
        }
    }

/*
    ret = gpiod_line_request_rising_edge_events(line[GPIO_RISING_EDGE_PIN], CONSUMER);
    if (ret < 0) {
        perror("Request event notification failed");
        gpiod_line_release(line[GPIO_RISING_EDGE_PIN]);
        gpiod_chip_close(chip);
        return 1;
    }

    ret = gpiod_line_request_falling_edge_events(line[GPIO_FALLING_EDGE_PIN], CONSUMER);
    if (ret < 0) {
        perror("Request event notification failed");
        gpiod_line_release(line[GPIO_FALLING_EDGE_PIN]);
        gpiod_chip_close(chip);
        return 1;
    }
*/

    ret = gpiod_line_request_both_edges_events(line[GPIO_BOTH_EDGES_PIN], CONSUMER);
    if (ret < 0) {
        perror("Request event notification failed");
        gpiod_line_release(line[GPIO_BOTH_EDGES_PIN]);
        gpiod_chip_close(chip);
        return 1;
    }

    printf("Waiting for edge event\n");
    int pinnum = GPIO_BOTH_EDGES_PIN;
    while (1) {
        ret = gpiod_line_event_wait(line[pinnum], NULL);
        if (ret < 0) {
            perror("Wait for event failed");
            break;
        }

        ret = gpiod_line_event_read(line[pinnum], &event);
        if (ret < 0) {
            perror("Read event failed");
            break;
        }

        if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
            printf("Rising edge detected on pin %d\n", pinnum);
        } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
            printf("Falling edge detected on pin %d\n", pinnum);
        }
    }

    // Release lines and close chip
    for (int i = 0; i < GPIO_CHIP_PIN_COUNT; i++) {
        gpiod_line_release(line[i]);
    }
    gpiod_chip_close(chip);

    return 0;
}

