use eframe::egui;
use std::sync::{Arc, Mutex};
use std::time::Duration;

pub struct MyApp {
    is_open: bool,
    input: String,
    send_data: Arc<Mutex<String>>,
    recv_log: Arc<Mutex<String>>,
    /* 波特率 */
    selected_baud: String,
    baud_options: Vec<String>,
    baud: Arc<Mutex<String>>,
    /* 帧类型 */
    selected_frame_type: String,
    frame_type_options: Vec<String>,
    frame_type: Arc<Mutex<String>>,
    /* 帧id */
    selected_frame_id: String,
    frame_id: Arc<Mutex<String>>,
    /* 数据类型 */
    selected_data_type: String,
    data_type_options: Vec<String>,
    data_type: Arc<Mutex<String>>,
    /* 数据长度 */
    selected_dlc: String,
    dlc_options: Vec<String>,
    dlc: Arc<Mutex<String>>,

    selected_work_mode: String,
    work_mode_options: Vec<String>,
    work_mode: Arc<Mutex<String>>,

    open_device: Arc<Mutex<String>>,
}

impl MyApp {
    pub fn new(
        send_data: Arc<Mutex<String>>,
        recv_log: Arc<Mutex<String>>,
        frame_id: Arc<Mutex<String>>,
        baud: Arc<Mutex<String>>,
        frame_type: Arc<Mutex<String>>,
        data_type: Arc<Mutex<String>>,
        dlc: Arc<Mutex<String>>,
        work_mode: Arc<Mutex<String>>,
        open_device: Arc<Mutex<String>>,
    ) -> Self {
        {
            let mut id = frame_id.lock().unwrap();
            *id = "0000000".to_string(); // 默认值为 0
        }
        Self {
            is_open: false,
            input: String::new(),
            send_data,
            recv_log,
            selected_baud: "1000".to_string(),
            baud_options: vec![
                "1000".to_string(),
                "500".to_string(),
                "250".to_string(),
                "125".to_string(),
                "100".to_string(),
            ],
            baud,
            selected_frame_type: "standard".to_string(),
            frame_type_options: vec!["standard".to_string(), "extended".to_string()],
            frame_type,
            selected_frame_id: "frame id".to_string(),
            frame_id,
            selected_data_type: "data".to_string(),
            data_type_options: vec!["data".to_string(), "remote".to_string()],
            data_type,
            selected_dlc: "8".to_string(),
            dlc_options: vec![
                "0".to_string(),
                "1".to_string(),
                "2".to_string(),
                "3".to_string(),
                "4".to_string(),
                "5".to_string(),
                "6".to_string(),
                "7".to_string(),
                "8".to_string(),
            ],
            dlc,

            selected_work_mode: "Normal".to_string(),
            work_mode_options: vec![
                "Normal".to_string(),
                "LoopBack".to_string(),
                "Silent".to_string(),
                "Silent_LoopBack".to_string(),
            ],
            work_mode,
            open_device,
        }
    }
}

impl eframe::App for MyApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        let mut flag_updata = false;

        // Bottom panel: 发送窗口固定在底部
        egui::TopBottomPanel::bottom("send_panel").show(ctx, |ui| {
            ui.separator();
            ui.heading("📤 Send");

            // 第一行：波特率、帧类型、帧ID
            ui.horizontal(|ui| {
                let label_width = 70.0;
                let combo_width = 100.0;

                ui.label("Baud Rate:").on_hover_text("选择波特率");
                egui::ComboBox::from_id_source("baud_rate_selector")
                    .selected_text(&self.selected_baud)
                    .width(combo_width)
                    .show_ui(ui, |ui| {
                        for baud in &self.baud_options {
                            if ui
                                .selectable_value(&mut self.selected_baud, baud.clone(), baud)
                                .changed()
                            {
                                if let Ok(mut b) = self.baud.lock() {
                                    *b = baud.clone(); // ✅ 上报 baud
                                }
                            }
                        }
                    });

                ui.add_space(8.0); // 控制间距

                ui.label("Frame Type:").on_hover_text("帧格式");
                egui::ComboBox::from_id_source("frame_type_selector")
                    .selected_text(&self.selected_frame_type)
                    .width(combo_width)
                    .show_ui(ui, |ui| {
                        for ft in &self.frame_type_options {
                            if ui
                                .selectable_value(&mut self.selected_frame_type, ft.clone(), ft)
                                .changed()
                            {
                                if let Ok(mut f) = self.frame_type.lock() {
                                    *f = ft.clone(); // ✅ 上报 frame_type
                                }
                            }
                        }
                    });

                ui.add_space(8.0);

                ui.label("Frame ID:");
                let mut id = self.frame_id.lock().unwrap();
                ui.add_sized([80.0, 24.0], egui::TextEdit::singleline(&mut *id));
            });

            // 第二行：数据类型、DLC
            ui.horizontal(|ui| {
                let combo_width = 100.0;

                ui.label("Data Type:");
                egui::ComboBox::from_id_source("data_type_selector")
                    .selected_text(&self.selected_data_type)
                    .width(combo_width)
                    .show_ui(ui, |ui| {
                        for dt in &self.data_type_options {
                            if ui
                                .selectable_value(&mut self.selected_data_type, dt.clone(), dt)
                                .changed()
                            {
                                if let Ok(mut d) = self.data_type.lock() {
                                    *d = dt.clone(); // ✅ 上报 data_type
                                }
                            }
                        }
                    });

                ui.add_space(8.0);

                ui.label("DLC:");
                egui::ComboBox::from_id_source("dlc_selector")
                    .selected_text(&self.selected_dlc)
                    .width(combo_width)
                    .show_ui(ui, |ui| {
                        for dlc in &self.dlc_options {
                            if ui
                                .selectable_value(&mut self.selected_dlc, dlc.clone(), dlc)
                                .changed()
                            {
                                if let Ok(mut d) = self.dlc.lock() {
                                    *d = dlc.clone(); // ✅ 上报 dlc
                                }
                            }
                        }
                    });

                ui.label("Mode:");
                egui::ComboBox::from_id_source("mode_selector")
                    .selected_text(&self.selected_work_mode)
                    .width(combo_width)
                    .show_ui(ui, |ui| {
                        for mode in &self.work_mode_options {
                            if ui
                                .selectable_value(&mut self.selected_work_mode, mode.clone(), mode)
                                .changed()
                            {
                                if let Ok(mut d) = self.work_mode.lock() {
                                    *d = mode.clone(); // ✅ 上报 dlc
                                }
                            }
                        }
                    });
            });

            ui.horizontal(|ui| {
                let max_chars = self.selected_dlc.parse::<usize>().unwrap_or(0) * 2;

                // 去除空格，保留合法 HEX 字符
                let mut cleaned: String = self
                    .input
                    .chars()
                    .filter(|c| c.is_ascii_hexdigit())
                    .collect();

                // 截断为合法最大长度
                if cleaned.len() > max_chars {
                    cleaned.truncate(max_chars);
                }

                // 实时格式化（每两个字符加一个空格）
                let mut formatted = String::new();
                for (i, c) in cleaned.chars().enumerate() {
                    if i > 0 && i % 2 == 0 {
                        formatted.push(' ');
                    }
                    formatted.push(c);
                }

                // 临时存储用户编辑的字符串
                let mut edit_text = formatted.clone();

                ui.label("Data:");
                let response =
                    ui.add_sized([300.0, 24.0], egui::TextEdit::singleline(&mut edit_text));

                if response.changed() {
                    // 用户改了输入框，重新提取 hex 内容（不管光标）
                    let new_cleaned: String = edit_text
                        .chars()
                        .filter(|c| c.is_ascii_hexdigit())
                        .collect();

                    // 更新 self.input（内部存储）
                    self.input = new_cleaned.clone();
                }

                if ui.button("📤").clicked() {
                    let mut buf = self.send_data.lock().unwrap();
                    *buf = edit_text.clone(); // ✅ 含空格版本，用于 parse_hex_input
                }

                if self.is_open {
                    // 已打开，显示绿色“Close”按钮
                    let button = egui::Button::new("Close")
                        .fill(egui::Color32::from_rgb(200, 255, 200)) // 背景淡绿色
                        .stroke(egui::Stroke::new(1.5, egui::Color32::GREEN)); // 绿色边框

                    if ui.add(button).clicked() {
                        self.is_open = false;
                        let mut buf = self.open_device.lock().unwrap();
                        *buf = "close_device".to_string(); // 或清空
                    }
                } else {
                    // 未打开，显示“Open”按钮
                    if ui.button("Open").clicked() {
                        self.is_open = true;
                        let mut buf = self.open_device.lock().unwrap();
                        *buf = "open_device".to_string();
                    }
                }
            });
        });

        // Central panel: 可滚动的接收内容
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("📥 Receive");
            egui::ScrollArea::vertical().show(ui, |ui| {
                let log = self.recv_log.lock().unwrap();
                ui.label(log.as_str());
            });
        });

        ctx.request_repaint_after(Duration::from_millis(100));
    }
}
