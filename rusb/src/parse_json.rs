/* json配置文件 */
use serde::Deserialize;
use std::fs::File;
use std::io::{BufReader, Error};

#[derive(Debug, Deserialize)]
pub struct CanBaud {
    pub baud: u32,
    pub sjw: u8,
    pub psc: u16,
    pub bs1: u8,
    pub bs2: u8,
}

#[derive(Debug, Deserialize)]
pub struct CanFilter {
    pub number: u8,
    pub mode: u8,
    pub scale: u8,
    pub id_high: String,
    pub id_low: String,
    pub mask_id_high: String,
    pub mask_id_low: String,
}

#[derive(Debug, Deserialize)]
pub struct CanConfig {
    pub can_baud: CanBaud,
    pub work_mode: String,
    pub frame_type: String,
    pub frame_id: u32,
    pub data_type: String,
    pub dlc: u8,
    pub time_stamp: u8,
    pub receive_fifo_num: u8,

    pub filter: CanFilter,
}

pub fn start_init(path: &str) -> std::io::Result<CanConfig> {
    println!("开始从本地Json中加载用户信息.....");

    //打开指定的文件
    let fp = File::open(path)?;
    let reader = BufReader::new(fp);

    let config: CanConfig = serde_json::from_reader(reader)?;

    println!("{:#?}", config);

    Ok(config)
}
