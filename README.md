# STM32 数字钟（硬件工程）

基于 STM32F103C8T6 核心板的 4 位数码管数字钟。原理图、PCB、DRC、自动布线、铺铜均已完成，并通过嘉立创打样 + BOM 配单下单。

- **版本**：`v1.0-frozen`（已冻结的第一版，设计与下单参数一致、可复现）
- **冻结日期**：2026-10-03
- **设计工具**：嘉立创EDA 专业版 V4.1.60

---

## 一、板级参数

| 项目 | 参数 |
|---|---|
| 层数 | 2 层（双面） |
| 板材 / 板厚 | FR4 / 1.6 mm |
| 尺寸 | 95 × 75 mm（9.5 × 7.5 cm） |
| 阻焊 / 表面处理 | 绿油 / 无铅 OSP |
| 最小线宽 / 间距 | ≥ 5 mil |
| 过孔 | 0.3 mm 孔 / 0.6 mm 盘（12 / 24 mil） |
| 焊盘距板边 | ≥ 64 mil（1.6 mm） |

## 二、系统框图

模块级拓扑，一眼看清各功能块与主控的挂接关系（矢量源文件：[`docs/system_block_diagram.svg`](docs/system_block_diagram.svg)）。

![系统框图](docs/system_block_diagram.svg)

## 三、功能模块

- **显示**：4 位共阳数码管 SR410361N；段选串 100Ω 限流电阻，位选用 S8550 PNP 三极管高边驱动
- **输入**：4 路独立按键（10kΩ 上拉 + 100nF 去抖）
- **提示**：蜂鸣器（S8050 低边驱动）、4 路 LED 指示（灌电流驱动）
- **走时**：RTC + CR2032 备电，采用双二极管「或门」供电切换
- **接口**：USART1 三针排针（PA9 / PA10 / GND）、5V 输入两针排针
- **主控**：立创·地阔星 LCKFB-DKX-STM32F103C8T6 核心板

## 四、BOM

共 **22 行 / 21 种物料 / 57 个元件**，立创商城配单 **100% 匹配**，整单 **¥86.96**。

完整清单见 [`manufacture/clock_BOM.csv`](manufacture/clock_BOM.csv)，每行都带立创 C 编码，可直接导入立创商城 BOM 配单页一键下单。

关键器件：

| 器件 | 型号 | 立创编号 |
|---|---|---|
| 主控核心板 | LCKFB-DKX-STM32F103C8T6 | C22396880 |
| 数码管 | SR410361N（4 位共阳，12 脚） | C132660 |
| 蜂鸣器 | TMB09A05（Φ9 mm，脚距 5.0 mm） | C49246939 |
| 电池座 | KH-CR2032-2-1（插件式） | C5365915 |
| 轻触开关 | K2-1102SP-A4SC-04（6×6×4.3） | C83916 |

## 五、制造与下单记录

| 时间 | 订单号 | 内容 | 金额 |
|---|---|---|---|
| 2026-10-02 23:28 | `clock_PCB1_20261002_232810` | PCB 光板 5 片（双面 / 1.6mm / 绿油 / 无铅 OSP） | ¥0.00（免费打样） |
| 2026-10-03 10:35 | `clock_PCB1_20261003_103524` | PCB 光板 5 片 | ¥30.00（预估） |
| 2026-10-03 | — | 元器件 BOM 一键下单（100% 匹配） | ¥86.96 |

采购方案为「**光板打样 + 立创商城配单 + 手工焊接**」。

## 六、目录结构

```
.
├── README.md
├── .gitignore
├── src/
│   └── stm32-digital-clock_v1.0.epro2   # 嘉立创EDA 源工程（原理图 + PCB + 面板）
├── manufacture/
│   ├── clock_Gerber.zip                 # Gerber + 钻孔文件，可直接上传打板
│   └── clock_BOM.csv                    # 100% 匹配 BOM，可直接导入商城下单
└── docs/                                # 最终状态截图
    ├── system_block_diagram.svg         # 系统框图（矢量源文件）
    ├── 01_layout.png
    ├── 02_routed.png
    ├── 03_poured_final.png
    └── schematic/                       # 原理图分块截图
        ├── 00_schematic_overview.png    # 2 倍高清总览
        ├── 00_contact_sheet.png         # 分块联络表（一览）
        ├── 00_schematic_full.svg        # 矢量源图
        └── 01_usart.png … 07_buzzer_driver.png
```

## 七、复现步骤

1. 用嘉立创EDA 专业版「文件 → 导入工程」打开 `src/stm32-digital-clock_v1.0.epro2`
2. 打开原理图，跑一次 ERC；打开 PCB，跑一次 DRC
3. 确认铺铜已重建（GND 铜皮）
4. 「制造 → 导出 Gerber」+「导出钻孔文件」，或直接用 `manufacture/clock_Gerber.zip` 上传打板
5. 用 `manufacture/clock_BOM.csv` 在立创商城 BOM 配单页下单买料
6. 手工焊接、上电调试

## 八、注意事项

- `.epro2` / `.zip` / `.png` 都是二进制，Git 无法 diff，**版本追溯依靠 tag**（本版为 `v1.0-frozen`）
- 蜂鸣器 TMB09A05 是 **5V 有源电磁式**，用 3V3 驱动时音量偏小；如需更响可换 3V 型号（但脚距须保持 5.0 mm，否则焊不上）
- 本版按**手工焊接**设计，器件最大为 0603 / 0805 / SOT-23 与插件；后续若加入 QFN / BGA，需转 SMT 一站式
- 打样已完成的板子，任何元件替换**都不能改封装 / 脚距**

## 九、效果图

| 布局完成 | 自动布线完成 | 铺铜完成（最终） |
|---|---|---|
| ![布局](docs/01_layout.png) | ![布线](docs/02_routed.png) | ![铺铜](docs/03_poured_final.png) |

## 十、原理图分块

按功能模块拆分的原理图截图，便于对照 BOM 与 PCB 逐块核对。完整总览见 [`docs/schematic/00_schematic_overview.png`](docs/schematic/00_schematic_overview.png)，分块一览见联络表：

![原理图分块联络表](docs/schematic/00_contact_sheet.png)

| # | 模块 | 截图 |
|---|---|---|
| 01 | USART 接口 | ![USART](docs/schematic/01_usart.png) |
| 02 | RTC 备份供电 | ![RTC Backup Power](docs/schematic/02_rtc_backup_power.png) |
| 03 | 5V 输入 / 主控核心板 | ![5V Input](docs/schematic/03_5v_input.png) |
| 04 | 独立按键 | ![User Keys](docs/schematic/04_user_keys.png) |
| 05 | 状态 LED | ![State LEDs](docs/schematic/05_state_leds.png) |
| 06 | 数码管驱动 | ![Digit Driver](docs/schematic/06_digit_driver.png) |
| 07 | 蜂鸣器驱动 | ![Buzzer Driver](docs/schematic/07_buzzer_driver.png) |
