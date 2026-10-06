# STM32 数字钟（硬件工程）

基于 STM32F103C8T6 核心板的 4 位数码管数字钟。原理图、PCB、DRC、自动布线、铺铜均已完成，并通过嘉立创打样 + BOM 配单下单。

本仓库保存同一原理图的**两版 PCB 设计**：原理图自 V1 起冻结不变，PCB 每版从头重新设计。

| 版本 | tag | 冻结日期 | 说明 |
|---|---|---|---|
| V1 | `v1.0-frozen` | 2026-10-03 | 首版 PCB，产物在仓库根目录 |
| V2 | `v2.0-frozen` | 2026-10-04 | PCB 全面重设计，产物在 `*/v2/` 子目录，详见第十一章 |

- **设计工具**：嘉立创EDA 专业版 V4.1.60

---

## 零、开发总流程（最终版）

> 原则：**IOC 图形化配置 + AI 交叉检查通过后，才开始写代码。**

```text
嘉立创EDA：原理图（引脚分配定稿）→ PCB → ERC / DRC → 打 tag 冻结硬件版本
   ↓
导出四件套放进同一个文件夹：功能需求.md、.epro2、原理图 PDF / 截图（BOM、网表可选）
   ↓
CubeMX：时钟树 + 激活外设 + 处理 Debug/JTAG → 生成 HAL 工程（即“新建空文件夹”）
   ↓
AI 交叉验证：网表 ↔ .ioc ↔ 截图 三方互查，产出 ①比对报告 ②pinmap.h
   ↓
按 .ioc 修正意见重新生成（如需要）→ 定代码框架、写驱动骨架
   ↓
填应用逻辑，上板先跑引脚级自检（按键回显、全段点亮、蜂鸣一声），再上真逻辑
```

**下一步**：`.epro2` 原理图 → `CubeMX` 生成 HAL 工程，AI 三方比对产出 `pinmap.h`。

> **实战：一次 AI 交叉验证如何挖出 .ioc 的 bug（以本工程为例）**
>
> 三方比对时，判断先后从"最省事"到"最实锤"，顺序决定你能否发现隐藏错误：
>
> 1. **先读 `clock.ioc`** → 只知道方向（12 个输出、4 个输入），**看不出谁是谁**；
> 2. **再看原理图截图** `03_5v_input.png` → 拿到"标签 → 引脚"映射，注意 **`SEG_G` 排针 11 脚 = PB3**；
> 3. **解包 `.epro2` 解析 PCB 焊盘网表** → 40 个焊盘逐一实锤，**和截图一致**；
> 4. **拿这份实锤网表回头比对 `.ioc`** → 才发现 **`.ioc` 配了 PA3、漏了 PB3**；
> 5. **把核实结果 + 极性一起写进 `pinmap.h`**。
>
> ```text
> (.ioc: 只有方向) → (截图: 标签→引脚) → (.epro2网表: 40焊盘实锤)
>                                                            ↓
>                           回头比对 .ioc → 抓到 "PA3 重复 + PB3 漏配"
>                                                            ↓
>                                   核实 + 极性 → 写入 pinmap.h
> ```
>
> 结论：**先有鹰眼（截图方向）→ 再用探针（焊盘网表实锤）→ 最后照妖镜（回头比对 .ioc）**，三者缺一不可。

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

共 **22 行 / 22 种物料 / 59 件**（含地阔星核心板 1 件、1×20P 排母 2 件），立创商城配单 **100% 匹配**。

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
│   ├── stm32-digital-clock_v1.0.epro2   # V1 源工程（原理图 + PCB + 面板）
│   └── v2/
│       └── stm32-digital-clock_v2.0.epro2   # V2 源工程快照
├── manufacture/
│   ├── clock_Gerber.zip                 # V1 Gerber + 钻孔文件，可直接上传打板
│   ├── clock_BOM.csv                    # 100% 匹配 BOM，可直接导入商城下单
│   └── v2/
│       ├── clock_v2_Gerber.zip          # V2 Gerber（含泪滴）
│       └── clock_BOM.csv                # V2 BOM（与 V1 相同）
└── docs/                                # 最终状态截图
    ├── system_block_diagram.svg         # 系统框图（矢量源文件）
    ├── 01_layout.png
    ├── 02_routed.png
    ├── 03_poured_final.png
    ├── v2/                              # V2 PCB 冻结版截图
    │   ├── 00_full_board_all_layers.png
    │   ├── 01_TOP_copper.png
    │   ├── 02_BOTTOM_copper.png
    │   ├── 03_TOP_silkscreen.png
    │   ├── 04_power_rtc_zoom.png
    │   └── 05_mcu_dense_zoom.png
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
6. 手工焊接（顺序见下）、上电调试

**焊接顺序**（先贴片后插件、先矮后高）：

0603 阻容 → 0805 阻容 → SOT-23 三极管 → 0805 LED → 贴片轻触开关 → 插件二极管 → 排针 → 排母 → 电池座 → 数码管 → 蜂鸣器 → 最后插核心板（不焊）

> 排母先用核心板插上定位再焊，保证间距；LED / 二极管 / 数码管注意极性。

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

## 十一、V2.0 版本（PCB 重设计）

**背景**：原理图自 V1 冻结后未改动，V2 仅对 PCB 从头重新设计，目标是在走线效率、电源完整性与工艺细节上全面超过 V1。BOM 因原理图不变而完全沿用 V1（22 行 / 100% 匹配）。

**V2 产物位置**

| 内容 | 路径 |
|---|---|
| 工程快照 | [`src/v2/stm32-digital-clock_v2.0.epro2`](src/v2/stm32-digital-clock_v2.0.epro2) |
| Gerber（含泪滴） | [`manufacture/v2/clock_v2_Gerber.zip`](manufacture/v2/clock_v2_Gerber.zip) |
| BOM | [`manufacture/v2/clock_BOM.csv`](manufacture/v2/clock_BOM.csv) |
| 核对截图 | [`docs/v2/`](docs/v2/) |

**V1 → V2 指标对比**

| 指标 | V1 | V2 | 说明 |
|---|---|---|---|
| 板框 | 95 × 75 mm | 95 × 75 mm | 不变 |
| 元件布局 | 57 件（TOP 16 / BOTTOM 41） | 57 件（全 TOP） | 改单面布局 |
| 走线段数 | 563 | 453 | −20% |
| 走线总长 | 2715 mm | 1498 mm | −45% |
| 电源线宽 | 单一 10 mil | 10 / 15 / 20 / 25 / 30 mil 五档 | 按网络差异化 |
| 过孔 | 55 | 24 | −56% |
| 铺铜 | 1 区（底层 GND） | 2 区（TOP 3V3 + BOTTOM GND） | 双面整板铺铜 |
| 泪滴 | 0 | 296 | 强化焊盘/走线连接 |
| DRC | 通过 | 0 违规 | — |

**V2 效果图**

| 全层总览 | TOP 铜层 | BOTTOM 铜层 |
|---|---|---|
| ![全层](docs/v2/00_full_board_all_layers.png) | ![TOP](docs/v2/01_TOP_copper.png) | ![BOTTOM](docs/v2/02_BOTTOM_copper.png) |

| TOP 丝印 | 电源/RTC 局部 | MCU 密集区 |
|---|---|---|
| ![丝印](docs/v2/03_TOP_silkscreen.png) | ![电源](docs/v2/04_power_rtc_zoom.png) | ![MCU](docs/v2/05_mcu_dense_zoom.png) |

**复现**：导入 `src/v2/stm32-digital-clock_v2.0.epro2` → 跑 DRC（应为 0）→ 确认 TOP 3V3 / BOTTOM GND 铺铜已重建 → 直接用 `manufacture/v2/clock_v2_Gerber.zip` 上传打板。
