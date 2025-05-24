use anyhow::Error;
use rusb::UsbContext;
use rusb::{Context, TransferType};

static USBD_VID: u16 = 0xffff;
static USBD_PID: u16 = 0xffff;

pub struct UsbInfo {
    pid: u16,
    vid: u16,
    ep_in: u8,
    ep_out: u8,
    interface_num: u8,
    usb_device: Option<rusb::Device<Context>>,
    usb_handler: Option<rusb::DeviceHandle<Context>>,
}

impl UsbInfo {
    pub fn new() -> Self {
        UsbInfo {
            pid: std::default::Default::default(),
            vid: std::default::Default::default(),
            ep_in: std::default::Default::default(),
            ep_out: std::default::Default::default(),
            interface_num: std::default::Default::default(),
            usb_device: std::default::Default::default(),
            usb_handler: std::default::Default::default(),
        }
    }

    pub fn open(&mut self) -> bool {
        let context = Context::new().unwrap();

        //先通过字符串进行过滤
        let device_list = context.devices().expect("获取USB设备列表失败");
        let num_device = device_list.len();

        for dev in device_list.iter() {
            let device_desc = dev.device_descriptor().unwrap();

            match dev.open() {
                Ok(handle) => {
                    let Manufacturer_index = device_desc.manufacturer_string_index().unwrap();
                    let Product_name = device_desc.product_string_index().unwrap();

                    if let Ok(s) = handle.read_string_descriptor_ascii(Manufacturer_index) {
                        if !s.starts_with("Cherry") {
                            continue;
                        }
                        println!("{s}");
                        self.usb_handler = Some(handle);
                    }
                }
                Err(_) => {}
            };

            //得到 vid pid
            self.pid = dev.device_descriptor().unwrap().product_id();
            self.vid = dev.device_descriptor().unwrap().vendor_id();

            for config in dev.config_descriptor(0).iter() {
                for interface in config.interfaces().next().unwrap().descriptors() {
                    if interface.class_code() == 3 || interface.sub_class_code() == 2 {
                        if interface
                            .endpoint_descriptors()
                            .next()
                            .unwrap()
                            .transfer_type()
                            == TransferType::Interrupt
                        {
                            if self.vid == USBD_VID && self.pid == USBD_PID {
                                let mut address = interface.endpoint_descriptors().into_iter();
                                self.ep_in = address.next().unwrap().address();
                                self.ep_out = address.next().unwrap().address();
                                self.interface_num = interface.interface_number();
                                println!("vid = 0x{:x}, pid = 0x{:x}", self.vid, self.vid);
                                println!(
                                    "ep_in = 0x{:x}, ep_out = 0x{:x}",
                                    self.ep_in, self.ep_out
                                );

                                println!(
                                    "{:?}",
                                    interface
                                        .endpoint_descriptors()
                                        .next()
                                        .unwrap()
                                        .transfer_type()
                                );
                                self.usb_device = Some(dev);
                                return true;
                            }
                        }
                    }
                }
            }
        }
        false
    }

    pub fn write(&mut self, data: &[u8]) -> anyhow::Result<usize> {
        match self.usb_handler.as_ref().unwrap().write_interrupt(
            self.ep_out,
            data,
            std::time::Duration::from_millis(500),
        ) {
            Ok(len) => Ok(len),
            Err(e) => {
                println!("USB Error: {}", e);
                Ok(0)
            }
        }
    }

    pub fn read(&mut self, data: &mut [u8]) -> anyhow::Result<usize, Error> {
        match self.usb_handler.as_ref().unwrap().read_interrupt(
            self.ep_in,
            data,
            std::time::Duration::from_millis(500),
        ) {
            Ok(len) => Ok(len),
            Err(e) => Err(Error::from(e)),
        }
    }
}
