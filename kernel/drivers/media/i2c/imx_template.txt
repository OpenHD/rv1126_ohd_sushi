#include <linux/clk.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/rk-camera-module.h>
#include <media/media-entity.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include <linux/pinctrl/consumer.h>
#include <linux/rk-preisp.h>
#include "../platform/rockchip/isp/rkisp_tb_helper.h"

#define IMX499_NAME			"imx499"


// dummy new i2c camera driver

static int imx499_runtime_resume(struct device *dev)
{
    dev_err(dev,"imx499_runtime_resume\n");
    return 0;
}

static int imx499_runtime_suspend(struct device *dev)
{
    dev_err(dev,"imx499_runtime_suspend\n");
    return 0;
}

static const struct dev_pm_ops imx499_pm_ops = {
        SET_RUNTIME_PM_OPS(imx499_runtime_suspend,
                           imx499_runtime_resume, NULL)
};

static int imx499_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    struct device *dev = &client->dev;
    dev_err(dev,"imx499_probe\n");
    return 0;
}

static int imx499_remove(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    dev_err(dev,"imx499_remove\n");
    return 0;
}

static const struct of_device_id imx499_of_match[] = {
        { .compatible = "sony,imx415" },
        {},
};
MODULE_DEVICE_TABLE(of, imx499_of_match);

static const struct i2c_device_id imx499_match_id[] = {
        { "sony,imx499", 0 },
        { },
};

static struct i2c_driver imx499_i2c_driver = {
        .driver = {
                .name = IMX499_NAME,
                .pm = &imx499_pm_ops,
                .of_match_table = of_match_ptr(imx499_of_match),
        },
        .probe		= &imx499_probe,
        .remove		= &imx499_remove,
        .id_table	= imx499_match_id,
};

static int __init sensor_mod_init(void)
{
    return i2c_add_driver(&imx499_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
    i2c_del_driver(&imx499_i2c_driver);
}

device_initcall_sync(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("Sony imx499 sensor driver");
MODULE_LICENSE("GPL v2");