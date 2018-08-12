#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>


/*******************************************************************************
* i2c DRIVER
*******************************************************************************/

static int get_status(struct i2c_client *cl)
{
	int err, count;
	char buf[6];

	/**
	 * Read Status:
	 * -wait for 10ms (spacing to previous i2c command)
	 * -write 0x00
	 * -wait for 10ms
	 * -read 6bytes
	 */
	msleep(10);

	buf[0] = 0x00;
	count = 1;
	err = i2c_master_send(cl, buf, count);
	if (err != count)
		goto write_err;

	msleep(10);

	count = 6;
	err = i2c_master_recv(cl, buf, count);
	if (err != count)
		goto read_err;


	/* TODO: parse buffer */
	pr_alert("%x,%x,%x,%x,%x,%x\n",
		buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

	return 0;

/* error handling */
write_err:
	dev_err(&cl->dev, "%s: i2c write error (err=%d)\n", __func__, err);
	return err;

read_err:
	dev_err(&cl->dev, "%s: i2c read error (err=%d)\n", __func__, err);
	return err;
}

static int nunchuk_probe(struct i2c_client *cl, const struct i2c_device_id *id)
{
	int err, count;
	char buf[2];

	dev_info(&cl->dev, "%s called for client addr=%d, name=%s\n",
		__func__, cl->addr, cl->name);

	/**
	 * i2c Communication:
	 *
	 * READ:  <i2c_address> <register>
	 * WRITE: <i2c_address> <register> <value>
	 * (where i2c_address is send implicitly by the API)
	 *
	 *
	 * Initialization:
	 * - write 0x55 to register 0xf0
	 * - wait for 1ms
	 * - write 0x00 to register 0xfb
	 */
	buf[0] = 0xf0;
	buf[1] = 0x55;
	count = 2;
	err = i2c_master_send(cl, buf, count);
	if (err != count)
		goto write_err;

	msleep(1);

	buf[0] = 0xfb;
	buf[1] = 0x00;
	err = i2c_master_send(cl, buf, count);
	if (err != count)
		goto write_err;

	/* read status */
	err = get_status(cl);
	if (err)
		goto status_err;

	return 0;

/* error handling */
write_err:
	dev_err(&cl->dev, "%s: i2c write error (err=%d)\n", __func__, err);
	return err;

status_err:
	dev_err(&cl->dev, "%s: could not get status from nunchuk\n", __func__);
	return err;
}

static int nunchuk_remove(struct i2c_client *cl)
{
	dev_info(&cl->dev, "%s called\n", __func__);
	return 0;
}


/*******************************************************************************
* i2c DRIVER
*******************************************************************************/

static const struct of_device_id nunchuk_of_match[] = {
		{ .compatible = "nintendo,nunchuk" },
		{}
};
MODULE_DEVICE_TABLE(of, nunchuk_of_match);

/* fallback to OF style bus device match */
static const struct i2c_device_id nunchuk_id[] = {
	{"nunchuk", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, nunchuk_id);

static struct i2c_driver nunchuk_drv = {
	.driver = {
		.name = "nunchuk-driver",
		.owner = THIS_MODULE,
		.of_match_table = nunchuk_of_match
	},
	.probe = nunchuk_probe,
	.remove = nunchuk_remove,
	.id_table = nunchuk_id,
};


/*******************************************************************************
* MODULE HOOKS
*******************************************************************************/

static int __init nunchuk_start(void)
{
	printk(KERN_INFO "Loading module %s\n", THIS_MODULE->name);
	return i2c_add_driver(&nunchuk_drv);
}
module_init(nunchuk_start);

static void __exit nunchuk_end(void)
{
	printk(KERN_INFO "Unloading module %s\n", THIS_MODULE->name);
	i2c_del_driver(&nunchuk_drv);
}
module_exit(nunchuk_end);


/*******************************************************************************
* MODULE INFORMATION
*******************************************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Hartmann");
MODULE_DESCRIPTION("I2c nunchuk driver");
MODULE_VERSION(MOD_VER);
