#ifndef _i2c_nunchuk
#define _i2c_nunchuk


/*******************************************************************************
* MACROS
*******************************************************************************/

/* Parsing register values, buttons */
#define NUNCHUCK_BUTZ_UP BIT(0)
#define NUNCHUCK_BUTC_UP BIT(1)

/* Parsing register values, accelerometer */
#define ACCEL_ALIGN 2
#define ACCELX_SHIFT 2
#define ACCELY_SHIFT 4
#define ACCELZ_SHIFT 6


/*******************************************************************************
* DATA STRUCTURES
*******************************************************************************/

typedef struct
{
	char joy_x;
	char joy_y;
	u16 acc_x;
	u16 acc_y;
	u16 acc_z;
	bool c_button_down;
	bool z_button_down;
} nunchuk_status_t;

struct nunchuk_iio_priv {
	struct i2c_client *client;
};

#endif /* _i2c_nunchuk */