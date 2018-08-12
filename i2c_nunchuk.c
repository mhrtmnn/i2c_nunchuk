#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>


/*******************************************************************************
* i2c DRIVER
*******************************************************************************/

static int nunchuk_probe(struct i2c_client *cl, const struct i2c_device_id *id)
{
	dev_info(&cl->dev, "%s called\n", __func__);
	return 0;
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
