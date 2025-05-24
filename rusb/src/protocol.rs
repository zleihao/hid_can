pub mod protocol {
    pub enum Cmd {
        WorkMode = 0x02,
        FrameType = 0x03,
        DataType = 0x04,
        DataLength = 0x05,
        Timestamp = 0x06,
        FifoNumber = 0x07,
        FilterSet = 0x08,
        OpenDevice = 0x0A,
    }

    pub fn set_baud(buf: &mut [u8], baud: u32) {
        buf[0] = 0x01;
        buf[1] = 0x01;
        buf[2..6].copy_from_slice(&baud.to_le_bytes());
        buf[6] = 0;
        buf[7] = 0;
        buf[8] = 0;
        buf[9] = 0;
    }

    //设置工作模式
    //设置帧类型
    //设置数据类型
    //设置数据长度 General Settings
    pub fn general_settings(buf: &mut [u8], cmd: Cmd, mode: u8) {
        buf[0] = 0x01;
        buf[1] = cmd as u8;
        buf[2] = mode;
    }
}
