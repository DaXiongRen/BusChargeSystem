# BusChargeSystem

## 项目介绍

基于STM32F407+RFID的模拟公交车刷卡收费系统

做这个小小的项目过程中参考了很多正点原子的资料，很多东西都是刚开始学习，所以只会实现一些简单的增删改查功能。

## 1. 功能列表

| 功能 | 介绍 |
| :--- | :---- |
| 添加用户 | 刷卡识别需要添加的用户 |
| 删除用户 | 刷卡识别需要删除的用户 |
| 刷卡消费 | 每次刷卡消费金额为1元，同一用户可连续刷卡消费 |
| 余额充值 | 通过按键选择充值金额并刷卡识别完成充值 |

## 2. 项目开发

### 2.1 环境

| 环境        | 版本          |
| :---------- | :------------- |
| 操作系统    | Windows        |
| KeilMDK-ARM | 5.35          |

### 2.2 硬件端

- STM32F407
- RFID 模块

硬件接线方式

| STM32F407 | RFID 模块 |
| :-------- | :--------- |
| 3.3V      | VCC       |
| GND       | GND       |
| PC10      | CLK       |
| PC11      | MISO      |
| PC12      | MOSI      |
| PA4       | NSS       |
| PA6       | RST       |

### 2.3 部署
+ 使用Keil uVision5集成开发工具打开.\BusChargeSystem\USER\目录下的BusChargeSystem.uvprojx文件并编译
+ 编译后会在.\BusChargeSystem\OBJ\目录下生成BusChargeSystem.hex文件，通过FlyMcu工具将此文件下载到开发板上 或 使用DAP仿真器

## 5. 参考资料

[硬件资料](http://www.openedv.com/docs/boards/stm32/zdyz_stm32f407_explorer.html)

[正点原子@ALIENTEK](http://www.alientek.com/)