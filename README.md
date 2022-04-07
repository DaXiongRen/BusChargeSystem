# BusChargeSystem

## 项目介绍

基于 STM32F407+RFID 的模拟公交车刷卡收费系统

## 1. 功能列表

| 功能     | 介绍                                            |
| :------- | :---------------------------------------------- |
| 添加用户 | 刷卡识别需要添加的用户                          |
| 删除用户 | 刷卡识别需要删除的用户                          |
| 刷卡消费 | 每次刷卡消费金额为 1 元，同一用户可连续刷卡消费 |
| 余额充值 | 通过按键选择充值金额并刷卡识别完成充值          |

## 2. 项目开发

### 2.1 环境

| 环境        | 版本    |
| :---------- | :------ |
| 操作系统    | Windows |
| KeilMDK-ARM | 5.35    |

### 2.2 硬件端

- STM32F407
- RFID 模块
- TFT LCD 液晶屏模块

硬件接线方式

| STM32F407 | RFID 模块 |
| --------- | --------- |
| PA4       | SDA       |
| PC10      | SCK       |
| PC12      | MOSI      |
| PC11      | MISO      |
| GND       | GND       |
| PA6       | RST       |
| 3.3V      | VCC       |

TFT LCD 液晶屏模块 --- STM32F407 开发板上固定位置插入

### 2.3 部署

- 使用 Keil uVision5 集成开发工具打开.\BusChargeSystem\USER\目录下的 BusChargeSystem.uvprojx 文件并编译
- 编译后会在.\BusChargeSystem\OBJ\目录下生成 BusChargeSystem.hex 文件，通过 FlyMcu 工具将此文件下载到开发板上 或 使用 DAP 仿真器

## 5. 参考资料

[硬件资料](http://www.openedv.com/docs/boards/stm32/zdyz_stm32f407_explorer.html)

[正点原子@ALIENTEK](http://www.alientek.com/)
