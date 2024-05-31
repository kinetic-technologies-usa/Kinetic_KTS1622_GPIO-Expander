// SPDX-License-Identifier: GPL-2.0-only
/**
 * @brief	KTS1622 16 bit I2C bus I/O Expander
 * @author	Kinetic Technologies, San Jose, CA (https://www.kinet-ic.com/)
 * @date 	4/30/2024
 * @version 3.0
 * @note	Product brief and Datasheet: https://www.kinet-ic.com/kts1622/
 */

#include <linux/bits.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#include <asm/unaligned.h>

#include <../drivers/gpio/gpiolib.h>

/* KTS1622 definition */
#define NUM_PINS					(16)
#define NUM_PORTS					(2)
#define NUM_PINS_PER_PORT			(8)

/* Pin configuration */
#define PIN_OUTPUT					(0)
#define PIN_INPUT					(1)

#define PULL_UP_DOWN_DISABLE		(0)
#define PULL_UP_DOWN_ENABLE			(1)
#define PULL_DOWN					(0)
#define PULL_UP						(1)

/* Registers */
#define KTS1622_INPUT_0				(0x00)
#define KTS1622_INPUT_1				(0x01)
#define KTS1622_OUTPUT_0			(0x02)
#define KTS1622_OUTPUT_1			(0x03)
#define KTS1622_POLARITY_INVERSION_0	(0x04)
#define KTS1622_POLARITY_INVERSION_1	(0x05)
#define KTS1622_CONFIG_0			(0x06)
#define KTS1622_CONFIG_1			(0x07)

#define KTS1622_DRIVE_STRENGTH_0A	(0x40)
#define KTS1622_DRIVE_STRENGTH_0B	(0x41)
#define KTS1622_DRIVE_STRENGTH_1A	(0x42)
#define KTS1622_DRIVE_STRENGTH_1B	(0x43)
#define KTS1622_INPUT_LATCH_0		(0x44)
#define KTS1622_INPUT_LATCH_1		(0x45)
#define KTS1622_PULLUP_DOWN_ENABLE_0	(0x46)
#define KTS1622_PULLUP_DOWN_ENABLE_1	(0x47)
#define KTS1622_PULLUP_DOWN_SELECTION_0	(0x48)
#define KTS1622_PULLUP_DOWN_SELECTION_1	(0x49)
#define KTS1622_INTERRUPT_MASK_0	(0x4A)
#define KTS1622_INTERRUPT_MASK_1	(0x4B)
#define KTS1622_INTERRUPT_STATUS_0	(0x4C)
#define KTS1622_INTERRUPT_STATUS_1	(0x4D)

#define KTS1622_OUTPUT_PORT_CONFIG	(0x4F)
#define KTS1622_INTERRUPT_EDGE_0A	(0x50)
#define KTS1622_INTERRUPT_EDGE_0B	(0x51)
#define KTS1622_INTERRUPT_EDGE_1A	(0x52)
#define KTS1622_INTERRUPT_EDGE_1B	(0x53)
#define KTS1622_INTERRUPT_CLEAR_0	(0x54)
#define KTS1622_INTERRUPT_CLEAR_1	(0x55)
#define KTS1622_INPUT_STATUS_0		(0x56)
#define KTS1622_INPUT_STATUS_1		(0x57)
#define KTS1622_INDIVIDUAL_PIN_OUTPUT_0	(0x58)
#define KTS1622_INDIVIDUAL_PIN_OUTPUT_1	(0x59)
#define KTS1622_SWITCH_DEBOUNCE_ENABLE	(0x5A)

static const struct i2c_device_id kts1622_id[] = {
	{ "kts1622", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, kts1622_id);

struct kts1622_chip {
	struct i2c_client *client;
	struct gpio_chip gpio_chip;

	struct mutex i2c_lock;
	unsigned driver_data; /* Reserved */

	struct mutex irq_lock;
	struct irq_chip irq_chip;
	u8 irq_mask[2];
	u8 irq_edge[4];
	int irq_base;
};

static int kts1622_software_reset(struct kts1622_chip *chip)
{
	struct i2c_client *i2c = chip->client;
	int ret;
	u8 orig_addr = i2c->addr;

	i2c->addr = 0x00;
	/* Software reset command (0x06) */
	ret = i2c_smbus_write_byte(i2c, 0x06);
	i2c->addr = orig_addr;

	return ret;
}

static int kts1622_reg_write(struct kts1622_chip *chip, u8 reg_addr, u8 reg_val)
{
	struct i2c_client *i2c = chip->client;
	int ret;

	ret = i2c_smbus_write_byte_data(i2c, reg_addr, reg_val);

	return ret;
}

static int kts1622_reg_read(struct kts1622_chip *chip, u8 reg_addr, u8 *reg_val)
{
	struct i2c_client *i2c = chip->client;
	int ret;

	ret = i2c_smbus_read_byte_data(i2c, reg_addr);

	if (ret < 0)
		return ret;

	*reg_val = ret;
	return 0;
}

static int kts1622_reg_bit_set(struct kts1622_chip *chip, u8 reg_addr, u8 bit, u8 bit_val)
{
	u8 reg_val;
	int ret;

	ret = kts1622_reg_read(chip, reg_addr, &reg_val);
	if (ret < 0)
		return ret;

	if (bit_val)
		reg_val |= 1 << bit;
	else
		reg_val &= ~(1 << bit);

	ret = kts1622_reg_write(chip, reg_addr, reg_val);
	if (ret < 0)
		return ret;

	return 0;
}

static int kts1622_gpio_get_value(struct gpio_chip *gc, unsigned offset)
{
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	int port = offset / 8;
	int pin = offset % 8;
	u8 reg_val;
	int ret;

	mutex_lock(&chip->i2c_lock);
	ret = kts1622_reg_read(chip, KTS1622_INPUT_0 + port, &reg_val);
	mutex_unlock(&chip->i2c_lock);
	if (ret < 0)
		return ret;

	return !!(reg_val & (1 << pin));
}

static void kts1622_gpio_set_value(struct gpio_chip *gc, unsigned offset, int val)
{
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	int port = offset / 8;
	int pin = offset % 8;
	u8 reg_addr = KTS1622_OUTPUT_0 + port;

	mutex_lock(&chip->i2c_lock);
	kts1622_reg_bit_set(chip, reg_addr, pin, val ? 1 : 0);
	mutex_unlock(&chip->i2c_lock);
}

static int kts1622_gpio_set_direction(struct gpio_chip *gc, unsigned offset, int direction)
{
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	int port = offset / 8;
	int pin = offset % 8;
	u8 reg_addr = KTS1622_CONFIG_0 + port;
	int ret;

	mutex_lock(&chip->i2c_lock);

	if (direction == PIN_OUTPUT)
		ret = kts1622_reg_bit_set(chip, reg_addr, pin, 0);
	else
		ret = kts1622_reg_bit_set(chip, reg_addr, pin, 1);

	mutex_unlock(&chip->i2c_lock);

	return ret;
}

static int kts1622_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	return kts1622_gpio_set_direction(gc, offset, PIN_INPUT);
}

static int kts1622_gpio_direction_output(struct gpio_chip *gc, unsigned offset, int val)
{
	/* Set output value. */
	kts1622_gpio_set_value(gc, offset, val);

	/* Then, set output mode. */
	return kts1622_gpio_set_direction(gc, offset, PIN_OUTPUT);
}

static int kts1622_gpio_get_direction(struct gpio_chip *gc, unsigned offset)
{
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	int port = offset / 8;
	int pin = offset % 8;
	u8 reg_addr = KTS1622_CONFIG_0 + port;
	u8 reg_val;
	int ret;

	mutex_lock(&chip->i2c_lock);
	ret = kts1622_reg_read(chip, reg_addr, &reg_val);
	mutex_unlock(&chip->i2c_lock);

	if (ret < 0)
		return ret;

	return !!(reg_val & (1 << pin));
}

static int kts1622_gpio_set_pull_up_down(struct kts1622_chip *chip,
					 unsigned int offset,
					 unsigned long config)
{
	int port = offset / 8;
	int pin = offset % 8;
	u8 reg_en_addr = KTS1622_PULLUP_DOWN_ENABLE_0 + port;
	u8 reg_sel_addr = KTS1622_PULLUP_DOWN_SELECTION_0 + port;
	int ret;

	mutex_lock(&chip->i2c_lock);

	/* Disable pull-up/pull-down */
	ret = kts1622_reg_bit_set(chip, reg_en_addr, pin, PULL_UP_DOWN_DISABLE);
	if (ret < 0)
		goto exit;

	/* Configure pull-up/pull-down */
	if (config == PIN_CONFIG_BIAS_PULL_UP) {
		ret = kts1622_reg_bit_set(chip, reg_sel_addr, pin, PULL_UP);
		if (ret < 0)
			goto exit;
		ret = kts1622_reg_bit_set(chip, reg_en_addr, pin, PULL_UP_DOWN_ENABLE);

	} else if (config == PIN_CONFIG_BIAS_PULL_DOWN) {
		ret = kts1622_reg_bit_set(chip, reg_sel_addr, pin, PULL_DOWN);
		if (ret < 0)
			goto exit;
		ret = kts1622_reg_bit_set(chip, reg_en_addr, pin, PULL_UP_DOWN_ENABLE);

	} else if (config == PIN_CONFIG_BIAS_DISABLE) {
		ret = kts1622_reg_bit_set(chip, reg_en_addr, pin, PULL_UP_DOWN_DISABLE);
	}

	if (ret)
		goto exit;
exit:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int kts1622_gpio_set_open_drain(struct kts1622_chip *chip,
					 unsigned int offset,
					 unsigned long config)
{
	int port = offset / 8;
	int pin = offset % 8;
	u8 reg_addr = KTS1622_INDIVIDUAL_PIN_OUTPUT_0 + port;
	int ret;

	mutex_lock(&chip->i2c_lock);

	/* Configure Open-drain/Push-pull */
	if (config == PIN_CONFIG_DRIVE_OPEN_DRAIN)
		ret = kts1622_reg_bit_set(chip, reg_addr, pin, 0);
	else if (config == PIN_CONFIG_DRIVE_PUSH_PULL)
		ret = kts1622_reg_bit_set(chip, reg_addr, pin, 1);

	mutex_unlock(&chip->i2c_lock);

	return ret;
}

static int kts1622_gpio_set_config(struct gpio_chip *gc, unsigned int offset,
				   unsigned long config)
{
	struct kts1622_chip *chip = gpiochip_get_data(gc);

	switch (pinconf_to_config_param(config)) {
	case PIN_CONFIG_BIAS_PULL_UP:
	case PIN_CONFIG_BIAS_PULL_DOWN:
		return kts1622_gpio_set_pull_up_down(chip, offset, config);

	case PIN_CONFIG_DRIVE_OPEN_DRAIN:
	case PIN_CONFIG_DRIVE_PUSH_PULL:
		return kts1622_gpio_set_open_drain(chip, offset, config);

	default:
		return -ENOTSUPP;
	}
}

static void kts1622_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	int port = d->hwirq / 8;
	int pin = d->hwirq % 8;

	chip->irq_mask[port] |= 1 << pin;
}

static void kts1622_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	int port = d->hwirq / 8;
	int pin = d->hwirq % 8;

	chip->irq_mask[port] &= ~(1 << pin);
}

static void kts1622_irq_bus_lock(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct kts1622_chip *chip = gpiochip_get_data(gc);

	mutex_lock(&chip->irq_lock);
}

static void kts1622_irq_bus_sync_unlock(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	int port;

	/* Synchronize the register value */
	for (port=0; port<NUM_PORTS; port++) {
		kts1622_reg_write(chip, KTS1622_INTERRUPT_MASK_0 + port, chip->irq_mask[port]);
		kts1622_reg_write(chip, KTS1622_INTERRUPT_EDGE_0A + port*2, chip->irq_edge[port*2]);
		kts1622_reg_write(chip, KTS1622_INTERRUPT_EDGE_0A + port*2 + 1, chip->irq_edge[port*2 + 1]);
	}

	mutex_unlock(&chip->irq_lock);
}

static int kts1622_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	struct i2c_client *client = chip->client;
	u8 val;

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		val = 0x01;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		val = 0x02;
		break;

	case IRQ_TYPE_EDGE_BOTH:
		val = 0x03;
		break;

	case IRQ_TYPE_LEVEL_LOW:
	case IRQ_TYPE_LEVEL_HIGH:
		val = 0x00;
		break;

	default:
		dev_err(&client->dev, "unsupported irq type: %u\n", type);
		return -EINVAL;
	}

	chip->irq_edge[d->hwirq/4] &= ~(0x03 << (d->hwirq % 4));
	chip->irq_edge[d->hwirq/4] |= val << ((d->hwirq % 4) * 2);

	return 0;
}

static void kts1622_irq_shutdown(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct kts1622_chip *chip = gpiochip_get_data(gc);

	chip->irq_edge[d->hwirq/4] &= ~(0x03 << (d->hwirq % 4));
}

static irqreturn_t kts1622_irq_handler(int irq, void *devid)
{
	struct kts1622_chip *chip = devid;
	int nhandled = 0;
	int port = 0;
	int pin = 0;
	u8 irq_status[2];

	for (port = 0; port < NUM_PORTS; port++) {
		/* Read to check which line is the cause of the interrupt */
		kts1622_reg_read(chip, KTS1622_INTERRUPT_STATUS_0 + port, &irq_status[port]);

		/* Clear the interrupt flags */
		if (irq_status[port] != 0x00)
			kts1622_reg_write(chip, KTS1622_INTERRUPT_CLEAR_0 + port, irq_status[port]);
	}

	for (port = 0; port < NUM_PORTS; port++) {
		for (pin = 0; pin < NUM_PINS_PER_PORT; pin++) {
			if ((irq_status[port] >> pin) & 0x01) {
				handle_nested_irq(irq_find_mapping(chip->gpio_chip.irq.domain, port * NUM_PINS_PER_PORT + pin));
				nhandled++;
			}
		}
	}

	return (nhandled > 0) ? IRQ_HANDLED : IRQ_NONE;
}

static int kts1622_irq_setup(struct kts1622_chip *chip)
{
	struct i2c_client *client = chip->client;
	struct irq_chip *irq_chip = &chip->irq_chip;
	int port;
	int ret;

	if (!client->irq)
		return 0;

	if (chip->irq_base == -1)
		return 0;

	mutex_init(&chip->irq_lock);

	ret = devm_request_threaded_irq(&client->dev, client->irq,
					NULL, kts1622_irq_handler,
					IRQF_ONESHOT,
					dev_name(&client->dev), chip);
	if (ret) {
		dev_err(&client->dev, "failed to request irq %d\n",
			client->irq);
		return ret;
	}

	irq_chip->name = dev_name(&chip->client->dev);
	irq_chip->irq_mask = kts1622_irq_mask;
	irq_chip->irq_unmask = kts1622_irq_unmask;
	irq_chip->irq_bus_lock = kts1622_irq_bus_lock;
	irq_chip->irq_bus_sync_unlock = kts1622_irq_bus_sync_unlock;
	irq_chip->irq_set_type = kts1622_irq_set_type;
	irq_chip->irq_shutdown = kts1622_irq_shutdown;

	for (port=0; port<NUM_PORTS; port++) {
		kts1622_reg_read(chip, KTS1622_INTERRUPT_MASK_0 + port, &chip->irq_mask[port]);
		kts1622_reg_read(chip, KTS1622_INTERRUPT_EDGE_0A + port*2, &chip->irq_edge[port*2]);
		kts1622_reg_read(chip, KTS1622_INTERRUPT_EDGE_0A + port*2 + 1, &chip->irq_edge[port*2 + 1]);
	}

	ret =  gpiochip_irqchip_add_nested(&chip->gpio_chip, irq_chip,
					   chip->irq_base, handle_simple_irq,
					   IRQ_TYPE_NONE);
	if (ret) {
		dev_err(&client->dev,
			"could not connect irqchip to gpiochip\n");
		return ret;
	}

	gpiochip_set_nested_irqchip(&chip->gpio_chip, irq_chip, client->irq);

	return 0;
}

static void kts1622_debug_show(struct seq_file *s, struct gpio_chip *gc)
{
	struct kts1622_chip *chip = gpiochip_get_data(gc);
	u8 reg_addr;
	u8 reg_val;
	int ret;

	seq_puts(s, "regs:\n");
	for (reg_addr = 0; reg_addr <= 7; reg_addr++) {
		ret = kts1622_reg_read(chip, reg_addr, &reg_val);
		if (ret < 0)
			goto error;
		seq_printf(s, " 0x%02X: 0x%02X\n", reg_addr, reg_val);
	}
	for (reg_addr = 0x40; reg_addr <= 0x5A; reg_addr++) {
		if (reg_addr == 0x4E)
			continue; /* Reserved */

		ret = kts1622_reg_read(chip, reg_addr, &reg_val);
		if (ret < 0)
			goto error;

		seq_printf(s, " 0x%02X: 0x%02X\n", reg_addr, reg_val);
	}
	return;
error:
	seq_printf(s, "Failed to read KTS1622 registers (ret=%d).", ret);
}

static void kts1622_setup_gpio(struct kts1622_chip *chip)
{
	struct gpio_chip *gc;

	gc = &chip->gpio_chip;

	gc->direction_input  = kts1622_gpio_direction_input;
	gc->direction_output = kts1622_gpio_direction_output;
	gc->get = kts1622_gpio_get_value;
	gc->set = kts1622_gpio_set_value;
	gc->get_direction = kts1622_gpio_get_direction;
	gc->set_config = kts1622_gpio_set_config;
	gc->dbg_show = kts1622_debug_show;

	gc->base = -1;
	gc->can_sleep = false;
	gc->ngpio = 16;
	gc->label = dev_name(&chip->client->dev);
	gc->parent = &chip->client->dev;
	gc->owner = THIS_MODULE;
}

static int device_kts1622_init(struct kts1622_chip *chip)
{
	int ret = 0;

	mutex_lock(&chip->i2c_lock);

	/* Software reset */
	ret = kts1622_software_reset(chip);
	if (ret < 0)
		goto error;

	/* Default: Input mode (push-pull when output) */
	ret = kts1622_reg_write(chip, KTS1622_OUTPUT_PORT_CONFIG, 0x03);
	if (ret < 0)
		goto error;

	ret = kts1622_reg_write(chip, KTS1622_INDIVIDUAL_PIN_OUTPUT_0, 0xFF);
	if (ret < 0)
		goto error;

	ret = kts1622_reg_write(chip, KTS1622_INDIVIDUAL_PIN_OUTPUT_1, 0xFF);

error:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static const struct of_device_id kts1622_dt_ids[];

static int kts1622_probe(struct i2c_client *client,
			 const struct i2c_device_id *i2c_id)
{
	struct kts1622_chip *chip;
	int ret;

	/* Allocate, initialize, and register this gpio_chip. */
	chip = devm_kzalloc(&client->dev,
			sizeof(struct kts1622_chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->client = client;

	if (i2c_id) {
		chip->driver_data = i2c_id->driver_data;
	} else {
		const void *match;

		match = device_get_match_data(&client->dev);
		if (!match) {
			ret = -ENODEV;
			goto err_exit;
		}

		chip->driver_data = (uintptr_t)match;
	}

	i2c_set_clientdata(client, chip);

	kts1622_setup_gpio(chip);

	mutex_init(&chip->i2c_lock);

	ret = device_kts1622_init(chip);
	if (ret)
		goto err_exit;

	ret = devm_gpiochip_add_data(&client->dev, &chip->gpio_chip, chip);
	if (ret)
		goto err_exit;

	chip->irq_base = 0;
	ret = kts1622_irq_setup(chip);
	if (ret)
		goto err_exit;

	return 0;

err_exit:
	printk("Probe failed %d", ret);
	return ret;
}

static int kts1622_remove(struct i2c_client *client)
{
	/* Nothing to do for now. */
	return 0;
}

static const struct of_device_id kts1622_dt_ids[] = {
	{ .compatible = "kinetic_technologies,kts1622" },
	{ }
};

MODULE_DEVICE_TABLE(of, kts1622_dt_ids);

static struct i2c_driver kts1622_driver = {
	.driver = {
		.name	= "kts1622",
		.of_match_table = kts1622_dt_ids,
	},
	.probe		= kts1622_probe,
	.remove		= kts1622_remove,
	.id_table	= kts1622_id,
};

static int __init kts1622_init(void)
{
	return i2c_add_driver(&kts1622_driver);
}
/* register after i2c postcore initcall and before
 * subsys initcalls that may rely on these GPIOs
 */
subsys_initcall(kts1622_init);

static void __exit kts1622_exit(void)
{
	i2c_del_driver(&kts1622_driver);
}
module_exit(kts1622_exit);

MODULE_AUTHOR("KINETIC_TECHNOLOGIES");
MODULE_DESCRIPTION("16-bit GPIO expander driver for KTS1622");
MODULE_LICENSE("GPL");
