/**
 * @file main.h
 * @author DXR (daxiongren@foxmail.com)
 * @brief 主程序头文件
 * @version 2.2
 * @date 2021-12-02
 */
#ifndef __MAIN_H
#define __MAIN_H
#include "sys.h"

#define ID_LEN 4      // 用户卡ID长度(字节)
#define NAME_LEN 3    // 用户名长度(字节)
#define BALANCE_LEN 1 // 账户余额长度(字节)

/* 选择日期的模式 */
#define D 0
#define H 1
#define M 2
#define DHM 3

/* 定义布尔类型 */
typedef enum
{
    TRUE,
    FALSE
} Boolean;

/* 主程序变量定义 */
extern u8 adminID[ID_LEN];        // 管理员ID卡号
extern u8 cardID[ID_LEN + 1];     // 存放普通用户ID卡号
extern u8 userName[NAME_LEN + 1]; // 用户名
extern u8 balance;                // 账户余额
/* keyFun取值 -> KEY_NULL:无操作 KEY0_VALUE:运行读卡信息 KEY1_VALUE:添加用户卡 KEY2_VALUE:删除用户卡 */
extern u8 keyFun;

/* 主程序所有函数接口定义 */
void BCSInit(void);                           // DoorControlSystem(DCS)初始化
void SysRunning(void);                        // 系统运行入口
void ConsumeTips(void);                       // 消费提示框
void BCSRunning(void);                        // 运行消费
void LcdDesktop(void);                        // 用户桌面
void RechargeMenu(void);                      // 显示充值菜单
void BalanceRecharge(void);                   // 添加用户余额
u8 GetBalance(void);                          // 获取用户余额
void SetBalance(u8 balance);                  // 设置用户余额
Boolean SearchID(u8 *cardID, u32 *index);     // 搜索用户卡ID
Boolean IsEquals(u8 *arr1, u8 *arr2, u8 len); // 数组比较
Boolean IsNullID(u8 *cardID, u8 size);        // 判断ID是否为空
Boolean IsNullData(u8 *data, u8 size);        // 判断数据是否为空
void ReadCardTips(void);                      // 读卡提示信息
char ReadCard(void);                          // 读卡
void ManageUserMenu(void);                    // 管理用户菜单
void ManageUser(void);                        // 管理用户
void AddUserMenu(void);                       // 显示添加用户菜单
void DelUserMenu(void);                       // 显示删除用户菜单
void UserSignup(void);                        // 用户注册
void AddUser(u8 *cardID);                     // 添加用户
Boolean AddData(u8 *cardID);                  // 添加ID
void DeleteUser(void);                        // 删除用户
void RemoveAllUser(void);                     // 删除所有用户
void RemoveUser(u8 *cardID);                  // 删除指定用户
Boolean RemoveData(u8 *cardID);               // 删除指定用户数据
void RemoveAllData(u32 startAddr);            // 删除所有ID
u8 ShowTime(u8 timeMode);                     // 显示时间

#endif
