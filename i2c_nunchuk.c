#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input-polldev.h>
#include <linux/iio/iio.h>

#include "i2c_nunchuk.h"


/*******************************************************************************
* MACROS
*******************************************************************************/

#define NUNCHUK_POLL_INTERVAL 50


/*******************************************************************************
* MODULE PARAMETERS
*******************************************************************************/

static int dbg;
module_param(dbg, int, S_IRUGO);
MODULE_PARM_DESC(dbg, "Enable debug mode");


/*******************************************************************************
* HELPER FUNCTIONS
*******************************************************************************/

static int get_status(struct i2c_client *cl, nunchuk_status_t *status)
{
	int err, count;
	char buf[6];
	struct nunchuk_i2c_priv *i2c_data = i2c_get_clientdata(cl);

	mutex_lock(&i2c_data->mutex);

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

	mutex_unlock(&i2c_data->mutex);

	/**
	 * Parse the Buffer
	 *
	 *
	 * Joystick:
	 * buf[0] = joy_x[7:0]
	 * buf[1] = joy_y[7:0]
	 *
	 * Acceleration:
	 * buf[2] = acc_x[9:2]
	 * buf[3] = acc_y[9:2]
	 * buf[4] = acc_z[9:2]
	 *
	 * buf[5][7:6] = acc_x[1:0]
	 * buf[5][5:4] = acc_y[1:0]
	 * buf[5][3:2] = acc_z[1:0]
	 *
	 * Buttons:
	 * buf[5][1] = button_c_up
	 * buf[5][0] = button_z_up
	 */
	status->joy_x = buf[0];
	status->joy_y = buf[1];

	status->acc_x = (buf[2] << ACCEL_ALIGN);
	status->acc_y = (buf[3] << ACCEL_ALIGN);
	status->acc_z = (buf[4] << ACCEL_ALIGN);
	status->acc_x |= ((buf[5] >> ACCELX_SHIFT) & 0x03U);
	status->acc_y |= ((buf[5] >> ACCELY_SHIFT) & 0x03U);
	status->acc_z |= ((buf[5] >> ACCELZ_SHIFT) & 0x03U);


	status->c_button_down = !(buf[5] & NUNCHUCK_BUTC_UP);
	status->z_button_down = !(buf[5] & NUNCHUCK_BUTZ_UP);

	if (dbg)
		pr_alert("joy-x=%03d, joy-y=%03d, acc-x=%04d, acc-y=%04d, acc-z=%04d, but-c_down=%d, but-z_down=%d\n",
			status->joy_x,
			status->joy_y,
			status->acc_x,
			status->acc_y,
			status->acc_z,
			status->c_button_down,
			status->z_button_down);

	return 0;

/* error handling */
write_err:
	mutex_unlock(&i2c_data->mutex);
	dev_err(&cl->dev, "%s: i2c write error (err=%d)\n", __func__, err);
	return -EIO;

read_err:
	mutex_unlock(&i2c_data->mutex);
	dev_err(&cl->dev, "%s: i2c read error (err=%d)\n", __func__, err);
	return -EIO;
}

int initialize_nunchuk(struct i2c_client *cl)
{
	int err, count;
	char buf[2];
	struct nunchuk_i2c_priv *i2c_data = i2c_get_clientdata(cl);

	mutex_lock(&i2c_data->mutex);

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

	mutex_unlock(&i2c_data->mutex);

	return 0;

write_err:
	mutex_unlock(&i2c_data->mutex);
	dev_err(&cl->dev, "%s: i2c write error (err=%d)\n", __func__, err);
	return -EIO;
}


/*******************************************************************************
* INDUSTRIAL IO DRIVER
*******************************************************************************/

static int nunchuk_iio_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	int ret;
	nunchuk_status_t status = {};
	struct nunchuk_iio_priv *data = iio_priv(indio_dev);
	struct i2c_client *cl = data->client;

	/* only raw mode is supported */
	if (mask != IIO_CHAN_INFO_RAW)
		return -EINVAL;

	/* read data from nunchuk */
	ret = get_status(cl, &status);
	if (ret)
		return ret;

	/* data needs to be written to 'val', 'val2' is ignored */

	switch (chan->channel2) {
	case IIO_MOD_X:
		*val = status.acc_x;
		break;
	case IIO_MOD_Y:
		*val = status.acc_y;
		break;
	case IIO_MOD_Z:
		*val = status.acc_z;
		break;
	default:
		return -EINVAL;
	}

	return IIO_VAL_INT;
}

/****************************** driver structures *****************************/
#define NUNCHUK_IIO_CHAN(axis) {			\
	.type = IIO_ACCEL,				\
	.modified = 1,					\
	.channel2 = IIO_MOD_##axis,			\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	\
}

static const struct iio_chan_spec nunchuk_iio_channles[] = {
	NUNCHUK_IIO_CHAN(X),
	NUNCHUK_IIO_CHAN(Y),
	NUNCHUK_IIO_CHAN(Z),
};

static const struct iio_info nunchuk_iio_info = {
	.read_raw	= nunchuk_iio_read_raw,
};


/*******************************************************************************
* INPUT DRIVER
*******************************************************************************/

void nunchuk_poll(struct input_polled_dev *polled_input)
{
	int err;
	nunchuk_status_t status = {};
	struct i2c_client *cl = container_of(polled_input->input->dev.parent,
					     struct i2c_client, dev);

	err = get_status(cl, &status);
	if (err)
		goto status_err;

	/* report new input events as required */
	input_report_key(polled_input->input, BTN_Z, status.z_button_down);
	input_report_key(polled_input->input, BTN_C, status.c_button_down);

	input_report_abs(polled_input->input, ABS_X, status.joy_x);
	input_report_abs(polled_input->input, ABS_Y, status.joy_y);
	// TODO: accel inputs

	input_sync(polled_input->input);

	return;

status_err:
	dev_err(&cl->dev, "%s: could not get status from nunchuk\n", __func__);
}


/*******************************************************************************
* i2c DRIVER
*******************************************************************************/

static int nunchuk_probe(struct i2c_client *cl, const struct i2c_device_id *id)
{
	int err;
	struct input_dev *input;
	struct input_polled_dev *polled_input;
	struct iio_dev *indio_dev;
	struct nunchuk_iio_priv *iio_data;
	struct nunchuk_i2c_priv *i2c_data;

	dev_info(&cl->dev, "%s called for client addr=%d, name=%s\n",
		__func__, cl->addr, cl->name);

	/************************ i2c private drv data ************************/
	i2c_data = devm_kzalloc(&cl->dev, sizeof(*i2c_data), GFP_KERNEL);
	if (!i2c_data)
		goto alloc_err;

	/* init mutex used to synchronize i2c reads */
	mutex_init(&i2c_data->mutex);

	i2c_set_clientdata(cl, i2c_data);

	/*********************** nunchuk initialization ***********************/
	err = initialize_nunchuk(cl);
	if (err)
		goto init_err;

	/************************** input dev setup ***************************/
	polled_input = devm_input_allocate_polled_device(&cl->dev);
	if (!polled_input)
		goto alloc_err;

	/* setup polled input device */
	polled_input->poll = nunchuk_poll;
	polled_input->poll_interval = NUNCHUK_POLL_INTERVAL;

	/* setup input device */
	input = polled_input->input;
	input->name = "Wii Nunchuk";
	input->id.bustype = BUS_I2C;

	/**
	 * evbit: bitmap of types of events supported by the device, where
	 * - EV_KEY is used to describe state changes of key-like devices
	 * - EV_ABS events describe absolute changes in a property
	 *
	 * see: Documentation/input/event-codes.txt
	 */
	set_bit(EV_KEY, input->evbit);
	set_bit(EV_ABS, input->evbit);

	/**
	 * keybit: bitmap of keys/buttons this device has
	 */
	set_bit(BTN_C, input->keybit);
	set_bit(BTN_Z, input->keybit);

	/**
	 * absbit: bitmap of absolute axes for the device
	 */
	set_bit(ABS_X, input->absbit);
	set_bit(ABS_Y, input->absbit);

	/* setup joystick params */
	input_set_abs_params(input, ABS_X, 0x00, 0xff, 0, 0);
	if (!input->absinfo)
		goto alloc_err;
	input_set_abs_params(input, ABS_Y, 0x00, 0xff, 0, 0);

	/* register fully initialized polled input device */
	err = input_register_polled_device(polled_input);
	if (err)
		goto polled_reg_err;


	/*************************** iio dev setup ****************************/
	indio_dev = devm_iio_device_alloc(&cl->dev, sizeof(*iio_data));
	if (!indio_dev)
		goto alloc_err;

	/* iio -> i2c connection (for .read_raw()) */
	iio_data = iio_priv(indio_dev);
	iio_data->client = cl;

	/* i2c -> iio connection (for unregistering during i2c-remove) */
	i2c_data = i2c_get_clientdata(cl);
	i2c_data->iiodev = indio_dev;

	/**
	 * INFO:
	 * polled_dev -> i2c can be realized with 'container_of'
	 * i2c -> polled_dev not required, see devm_input_allocate_polled_device
	 */

	indio_dev->dev.parent = &cl->dev;
	indio_dev->info = &nunchuk_iio_info;
	indio_dev->name = "Nunchuk Accel";
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = nunchuk_iio_channles;
	indio_dev->num_channels = ARRAY_SIZE(nunchuk_iio_channles);

	err = iio_device_register(indio_dev);
	if (err)
		goto iio_reg_err;

	return 0;

/* error handling */
init_err:
	dev_err(&cl->dev, "%s: could not initialize nunchuk\n", __func__);
	return err;

alloc_err:
	dev_err(&cl->dev, "%s: could not allocate memory\n", __func__);
	return -ENOMEM;

polled_reg_err:
	dev_err(&cl->dev, "%s: could not register polled device\n", __func__);
	return err;

iio_reg_err:
	dev_err(&cl->dev, "%s: could not register iio device\n", __func__);
	return err;
}

static int nunchuk_remove(struct i2c_client *cl)
{
	struct nunchuk_i2c_priv *i2c_data = i2c_get_clientdata(cl);
	struct iio_dev *indio_dev = i2c_data->iiodev;

	dev_info(&cl->dev, "%s called\n", __func__);

	if (indio_dev)
		iio_device_unregister(indio_dev);

	return 0;
}

/****************************** driver structures *****************************/
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
