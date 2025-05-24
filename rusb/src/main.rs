mod myusb;
mod parse_json;
mod protocol;
mod ui;

use crate::myusb::UsbInfo;
use crate::ui::MyApp;
use anyhow::Result;
use std::time::Duration;

use crate::parse_json::CanConfig;
use crate::protocol::protocol::Cmd::{
    DataLength, DataType, FifoNumber, FilterSet, FrameType, OpenDevice, Timestamp, WorkMode,
};
use eframe::egui;
use protocol::protocol::*;
use std::sync::{Arc, Mutex};
use std::thread;

fn parse_hex_input(s: &str) -> Vec<u8> {
    s.split_whitespace()
        .filter_map(|b| u8::from_str_radix(b, 16).ok())
        .collect()
}

/* 发送默认配置 */
fn default_config(config: CanConfig, usb: Arc<Mutex<UsbInfo>>) {
    let mut buf = vec![0u8; 64];
    buf[0] = 1;

    //发送波特率
    buf[1] = 0x01;
    buf[2..6].copy_from_slice(&config.can_baud.baud.to_le_bytes());
    buf[6] = config.can_baud.sjw;
    buf[7..9].copy_from_slice(&config.can_baud.psc.to_le_bytes());
    buf[9] = config.can_baud.bs1;
    buf[10] = config.can_baud.bs2;

    println!("data: {:?}", buf);
    if let Err(e) = usb.lock().unwrap().write(&buf) {
        eprintln!("Write error: {e:?}");
    }

    //配置工作模式
    let mode = match config.work_mode.as_str() {
        "Normal" => 0,
        "LoopBack" => 1,
        "Silent" => 2,
        "Silent_LoopBack" => 3,
        _ => 0,
    };
    general_settings(&mut buf, WorkMode, mode);

    //设置帧类型
    let frame_type = match config.frame_type.as_str() {
        "standard" => 0,
        "extended" => 1,
        _ => 0,
    };
    general_settings(&mut buf, FrameType, frame_type);

    buf[3..7].copy_from_slice(&config.frame_id.to_le_bytes());

    println!("data: {:?}", buf);
    if let Err(e) = usb.lock().unwrap().write(&buf) {
        eprintln!("Write error: {e:?}");
    }

    //设置数据类型
    let data_type = match config.data_type.as_str() {
        "data" => 0,
        "remote" => 1,
        _ => 0,
    };
    general_settings(&mut buf, DataType, data_type);

    println!("data: {:?}", buf);
    if let Err(e) = usb.lock().unwrap().write(&buf) {
        eprintln!("Write error: {e:?}");
    }

    //设置dlc
    let dlc = config.dlc;
    general_settings(&mut buf, DataLength, dlc);

    println!("data: {:?}", buf);
    if let Err(e) = usb.lock().unwrap().write(&buf) {
        eprintln!("Write error: {e:?}");
    }

    //时间戳
    let time_stamp = config.time_stamp;
    general_settings(&mut buf, Timestamp, time_stamp);

    println!("data: {:?}", buf);
    if let Err(e) = usb.lock().unwrap().write(&buf) {
        eprintln!("Write error: {e:?}");
    }

    //接收FIFO
    let fifo_num = config.receive_fifo_num;
    general_settings(&mut buf, FifoNumber, fifo_num);

    println!("data: {:?}", buf);
    if let Err(e) = usb.lock().unwrap().write(&buf) {
        eprintln!("Write error: {e:?}");
    }

    //滤波配置
    buf[0] = 1;
    buf[1] = FilterSet as u8;
    buf[2] = config.filter.number;
    buf[3] = config.filter.mode;
    buf[4] = config.filter.scale;

    let mut n = u16::from_str_radix(config.filter.id_high.trim_start_matches("0x"), 16).unwrap();
    buf[5..7].copy_from_slice(&n.to_le_bytes());
    n = u16::from_str_radix(config.filter.id_low.trim_start_matches("0x"), 16).unwrap();
    buf[7..9].copy_from_slice(&n.to_le_bytes());

    n = u16::from_str_radix(config.filter.mask_id_high.trim_start_matches("0x"), 16).unwrap();
    buf[9..11].copy_from_slice(&n.to_le_bytes());
    n = u16::from_str_radix(config.filter.mask_id_low.trim_start_matches("0x"), 16).unwrap();
    buf[11..13].copy_from_slice(&n.to_le_bytes());

    println!("data: {:?}", buf);
    if let Err(e) = usb.lock().unwrap().write(&buf) {
        eprintln!("Write error: {e:?}");
    }
}

fn main() -> Result<(), eframe::Error> {
    let send_data = Arc::new(Mutex::new(String::new()));
    let recv_log = Arc::new(Mutex::new(String::new()));
    let frame_id = Arc::new(Mutex::new(String::new()));

    let baud = Arc::new(Mutex::new(String::new()));
    let frame_type = Arc::new(Mutex::new(String::new()));
    let data_type = Arc::new(Mutex::new(String::new()));
    let dlc = Arc::new(Mutex::new(String::new()));
    let work_mode = Arc::new(Mutex::new(String::new()));
    let open_device = Arc::new(Mutex::new(String::new()));

    let baud_clone = Arc::clone(&baud);
    let work_mode_clone = Arc::clone(&work_mode);
    let frame_type_clone = Arc::clone(&frame_type);
    let frame_id_clone = Arc::clone(&frame_id);
    let data_type_clone = Arc::clone(&data_type);
    let dlc_clone = Arc::clone(&dlc);
    let open_device_clone = Arc::clone(&open_device);

    let send_data_clone = Arc::clone(&send_data);
    let recv_log_clone = Arc::clone(&recv_log);

    let usb = Arc::new(Mutex::new(UsbInfo::new()));

    if usb.lock().unwrap().open() {
        /* 把默认配置发送出去 */
        let path = "./config.json".to_string();
        if let Ok(can_canfig) = parse_json::start_init(&path) {
            //发送默认配置
            let usb_clone = usb.clone();
            default_config(can_canfig, usb_clone);
        } else {
            println!("json有误，默认配置失败....");
        }

        //发送数据
        let usb_write = Arc::clone(&usb);
        thread::spawn(move || loop {
            let data_opt = {
                let mut lock = send_data_clone.lock().unwrap();
                if !lock.is_empty() {
                    let bytes = parse_hex_input(&lock);
                    lock.clear();
                    Some(bytes)
                } else {
                    None
                }
            };

            if let Some(data) = data_opt {
                /* 获取帧ID */
                let mut id = 0u32;
                let mut id_lock = frame_id_clone.lock().unwrap();
                if !id_lock.is_empty() {
                    id = u32::from_str_radix(id_lock.as_str(), 16).unwrap();
                }

                let size = data.len();
                let mut buf = vec![0u8; 64];
                buf[0] = 0x01;
                buf[1] = 0x09;

                buf[2..6].copy_from_slice(&id.to_le_bytes());

                buf[6] = size as u8;

                buf[7..7 + size].copy_from_slice(&data[0..size]);

                println!("data: {:?}", buf);
                if let Err(e) = usb_write.lock().unwrap().write(&buf) {
                    eprintln!("Write error: {e:?}");
                }
            }

            //设置波特率
            let baud = {
                let mut lock = baud_clone.lock().unwrap();
                if !lock.is_empty() {
                    let baud = u32::from_str_radix(lock.as_str(), 10).unwrap();
                    println!("Baud = {:x}", baud);
                    let mut bytes = vec![0u8; 64];
                    set_baud(&mut bytes, baud);
                    lock.clear();
                    Some(bytes)
                } else {
                    None
                }
            };

            if let Some(data) = baud {
                println!("data: {:?}", data);
                if let Err(e) = usb_write.lock().unwrap().write(&data) {
                    eprintln!("Write error: {e:?}");
                }
            }

            //设置工作模式
            let work_mode = {
                let mut lock = work_mode_clone.lock().unwrap();
                if !lock.is_empty() {
                    let mode = match lock.as_str() {
                        "Normal" => 0,
                        "LoopBack" => 1,
                        "Silent" => 2,
                        "Silent_LoopBack" => 3,
                        _ => 0,
                    };
                    println!("Mode = {:x}", mode);
                    let mut bytes = vec![0u8; 64];
                    general_settings(&mut bytes, WorkMode, mode);
                    lock.clear();
                    Some(bytes)
                } else {
                    None
                }
            };

            if let Some(data) = work_mode {
                println!("data: {:?}", data);
                if let Err(e) = usb_write.lock().unwrap().write(&data) {
                    eprintln!("Write error: {e:?}");
                }
            }

            //设置帧模式
            let frame_type = {
                let mut lock = frame_type_clone.lock().unwrap();
                if !lock.is_empty() {
                    let f_type = match lock.as_str() {
                        "standard" => 0,
                        "extended" => 1,
                        _ => 0,
                    };
                    println!("f_type = {:x}", f_type);
                    let mut bytes = vec![0u8; 64];
                    general_settings(&mut bytes, FrameType, f_type);

                    // /* 获取帧ID */
                    // let mut id = 0u32;
                    // let mut id_lock = frame_id_clone.lock().unwrap();
                    // if !id_lock.is_empty() {
                    //     id = u32::from_str_radix(id_lock.as_str(), 16).unwrap();
                    // }

                    // bytes[3..7].copy_from_slice(&id.to_le_bytes());

                    lock.clear();
                    Some(bytes)
                } else {
                    None
                }
            };

            if let Some(data) = frame_type {
                println!("data: {:?}", data);
                if let Err(e) = usb_write.lock().unwrap().write(&data) {
                    eprintln!("Write error: {e:?}");
                }
            }

            //设置数据类型
            let data_type = {
                let mut lock = data_type_clone.lock().unwrap();
                if !lock.is_empty() {
                    let d_type = match lock.as_str() {
                        "data" => 0,
                        "remote" => 1,
                        _ => 0,
                    };
                    println!("d_type = {:x}", d_type);
                    let mut bytes = vec![0u8; 64];
                    general_settings(&mut bytes, DataType, d_type);
                    lock.clear();
                    Some(bytes)
                } else {
                    None
                }
            };

            if let Some(data) = data_type {
                println!("data: {:?}", data);
                if let Err(e) = usb_write.lock().unwrap().write(&data) {
                    eprintln!("Write error: {e:?}");
                }
            }

            //设置数据长度
            let dlc = {
                let mut lock = dlc_clone.lock().unwrap();
                if !lock.is_empty() {
                    let _dlc = u8::from_str_radix(lock.as_str(), 16).unwrap();
                    println!("_dlc = {:x}", _dlc);
                    let mut bytes = vec![0u8; 64];
                    general_settings(&mut bytes, DataLength, _dlc);
                    lock.clear();
                    Some(bytes)
                } else {
                    None
                }
            };

            if let Some(data) = dlc {
                println!("data: {:?}", data);
                if let Err(e) = usb_write.lock().unwrap().write(&data) {
                    eprintln!("Write error: {e:?}");
                }
            }

            //打开设备
            let open_device = {
                let mut lock = open_device_clone.lock().unwrap();
                if !lock.is_empty() {
                    let device_state = match lock.as_str() {
                        "open_device" => 0,
                        "close_device" => 1,
                        _ => 0,
                    };
                    let mut bytes = vec![0u8; 64];
                    general_settings(&mut bytes, OpenDevice, device_state);
                    lock.clear();
                    Some(bytes)
                } else {
                    None
                }
            };

            if let Some(data) = open_device {
                println!("data: {:?}", data);
                if let Err(e) = usb_write.lock().unwrap().write(&data) {
                    eprintln!("Write error: {e:?}");
                }
            }

            thread::sleep(Duration::from_millis(500));
        });

        let recv_log_clone = Arc::clone(&recv_log);
        let usb = Arc::clone(&usb);
        thread::spawn(move || loop {
            let mut buf = vec![0u8; 64];
            match usb.lock().unwrap().read(&mut buf) {
                Ok(size) => {
                    // let line = format!("{:02X?}\n", &buf[..size]);
                    // recv_log_clone.lock().unwrap().push_str(&line);
                    let id_type = match buf[5] {
                        0 => "STD",
                        1 => "EXT",
                        _ => "",
                    };
                    let id_bytes = &buf[6..10]; // 假设 ID 是 4 字节（扩展 ID）
                    let id =
                        u32::from_le_bytes([id_bytes[0], id_bytes[1], id_bytes[2], id_bytes[3]]);

                    let data_type = match buf[10] {
                        0 => "DATA",
                        1 => "REMOTE",
                        _ => "",
                    };
                    let dlc = buf[11];
                    let data = &buf[12..(12 + dlc as usize).min(buf.len() - 12)];

                    let data_str = data
                        .iter()
                        .map(|b| format!("{:02X}", b))
                        .collect::<Vec<_>>()
                        .join(" ");

                    let line = format!(
                        "{} | {:08X} | {} | {:02X}  | {}\n",
                        id_type, id, data_type, dlc, data_str
                    );

                    recv_log_clone.lock().unwrap().push_str(&line);
                }
                Err(e) => {
                    // eprintln!("Read error: {e:?}");
                }
            }
            thread::sleep(Duration::from_millis(50));
        });
    } else {
        eprintln!("No matching USB device found");
    }

    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "USB GUI",
        options,
        Box::new(|_cc| {
            Box::new(MyApp::new(
                send_data,
                recv_log,
                frame_id,
                baud,
                frame_type,
                data_type,
                dlc,
                work_mode,
                open_device,
            ))
        }),
    )
}
