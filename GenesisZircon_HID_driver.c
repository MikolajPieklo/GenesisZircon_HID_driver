#include <linux/hid.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>

/* Device ID */
// static const struct hid_device_id my_hid_table[] = {
// 	{ HID_USB_DEVICE(0x248A,
// 			 /*0x5b2e*/ 0x8266) },
// 	{}
// };

struct GenesisZircon_data {
	struct input_dev *input;
	struct hid_device *hdev;
	uint8_t event;
};

static const struct hid_device_id GenesisZircon_HID_driver_table[] = {
	{ HID_BLUETOOTH_DEVICE(0x248A, 0x8266) },
	{}
};

MODULE_DEVICE_TABLE(hid, GenesisZircon_HID_driver_table);

static int GenesisZircon_HID_driver_input_configured(struct hid_device *hdev,
						     struct hid_input *hidinput)
{
	struct input_dev *input_dev = hidinput->input;
	struct GenesisZircon_data *zircon_data = hid_get_drvdata(hdev);

	if (!input_dev) {
		dev_err(&hdev->dev, "hid_input is not initialized!\n");
		return -EINVAL;
	}

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_SIDE, input_dev->keybit);
	set_bit(BTN_EXTRA, input_dev->keybit);
	set_bit(BTN_FORWARD, input_dev->keybit);
	set_bit(BTN_BACK, input_dev->keybit);
	zircon_data->input = input_dev;
	input_sync(input_dev);

	dev_info(&hdev->dev, "Key map is updated!\n");

	return 0;
}

static int GenesisZircon_HID_driver_raw_event(struct hid_device *hdev,
					      struct hid_report *report,
					      u8 *data, int size)
{
	// RM:   3  0
	// LPM:  3  1 0 0 0 0 0 0
	// PPM:  3  2 0 0 0 0 0 0
	// SPM:  3  4 0 0 0 0 0 0
	// EGPM: 3 10 0 0 0 0 0 0
	// EDPM: 3  8 0 0 0 0 0 0
	struct GenesisZircon_data *zircon_data = hid_get_drvdata(hdev);
	struct input_dev *input;

	if (!zircon_data) {
		dev_err(&hdev->dev, "Brak danych sterownika w raw_event()!\n");
		return -ENODEV;
	}
	input = zircon_data->input;

	// printk(KERN_INFO "Raw HID report data (size: %d): %*ph\n", size, size, data);

	if ((data[0] == 3) && (data[1] == 0x10)) {
		if (zircon_data->event == 0) {
			// printk(KERN_INFO "Raport BTN_FORWARD!\n");
			input_event(input, EV_MSC, MSC_SCAN, 0x90005);
			input_report_key(input, BTN_EXTRA, 1);
			zircon_data->event |= 0x01;
		}
	}
	if ((data[0] == 3) && (data[1] == 0x08)) {
		// printk(KERN_INFO "Raport BTN_BACK!\n");
		if (zircon_data->event == 0) {
			input_event(input, EV_MSC, MSC_SCAN, 0x90004);
			input_report_key(input, BTN_SIDE, 1);
			zircon_data->event |= 0x02;
		}
	}

	if ((data[0] == 3) && (data[1] != 0x10) &&
	    (zircon_data->event & 0x01)) {
		input_event(input, EV_MSC, MSC_SCAN, 0x90005);
		input_report_key(input, BTN_EXTRA, 0);
		zircon_data->event &= 0xFE;
	}
	if ((data[0] == 3) && (data[1] != 0x08) &&
	    (zircon_data->event & 0x02)) {
		input_event(input, EV_MSC, MSC_SCAN, 0x90004);
		input_report_key(input, BTN_SIDE, 0);
		zircon_data->event &= 0xFD;
	}
	input_sync(input);
	return 1;
}

static int GenesisZircon_HID_driver_probe(struct hid_device *hdev,
					  const struct hid_device_id *id)
{
	int ret;
	struct GenesisZircon_data *zircon_data;

	zircon_data =
		devm_kzalloc(&hdev->dev, sizeof(*zircon_data), GFP_KERNEL);
	if (!zircon_data) {
		return -ENOMEM;
	}
	dev_info(&hdev->dev, "Allocate memory!\n");

	hid_set_drvdata(hdev, zircon_data);

	// HID initialize
	ret = hid_parse(hdev);
	if (ret) {
		dev_err(&hdev->dev, "HID parse failed\n");
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		dev_err(&hdev->dev, "HID hw start failed\n");
		return ret;
	}
	zircon_data->hdev = hdev;
	zircon_data->event = 0;

	dev_info(&hdev->dev, "Genesis HID Drv is started!\n");

	return 0;
}

static void GenesisZircon_HID_driver_remove(struct hid_device *hdev)
{
	dev_info(&hdev->dev, "Genesis HID Drv is stopped!\n");
	hid_hw_stop(hdev);
}

static struct hid_driver GenesisZircon_HID_driver = {
	.name = "GenesisZircon_HID_driver",
	.id_table = GenesisZircon_HID_driver_table,
	.probe = GenesisZircon_HID_driver_probe,
	.remove = GenesisZircon_HID_driver_remove,
	.raw_event = GenesisZircon_HID_driver_raw_event,
	.input_configured = GenesisZircon_HID_driver_input_configured,
};

static int __init GenesisZircon_HID_driver_init(void)
{
	printk(KERN_INFO "GenesisZircon_HID_driver_init\n");
	return hid_register_driver(&GenesisZircon_HID_driver);
}

static void __exit GenesisZircon_HID_driver_exit(void)
{
	printk(KERN_INFO "GenesisZircon_HID_driver_exit\n");
	hid_unregister_driver(&GenesisZircon_HID_driver);
}

module_init(GenesisZircon_HID_driver_init);
module_exit(GenesisZircon_HID_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikolaj Pieklo");
MODULE_DESCRIPTION("Genesis Ziron 500 HID driver");
