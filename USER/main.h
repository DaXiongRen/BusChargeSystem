#ifndef _MAIN_H
#define _MAIN_H
#include "sys.h"

/* 用户卡ID长度 */
#define ID_LEN 4

/* 在数据存储区中用户卡ID存在的标志 */
#define EXIST 1

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
extern u8 keyFun; //keyFun取值 -> KEY_NULL:无操作 KEY0_VALUE:运行读卡信息 KEY1_VALUE:添加用户卡 KEY2_VALUE:删除用户卡
extern u8 CardID[ID_LEN];

/* 主程序所有函数接口 */
void DCSInit(void);                   // DoorControlSystem(DCS)初始化
void DCSRunning(void);                // 系统运行入口
void UserLogin(void);                 // 用户登录
void LcdDesktop(void);                // 用户桌面
u32 SerachID(u8 *CardID);             // 搜索用户卡ID
Boolean IsEquals(u8 *arr1, u8 *arr2); // 数组比较
u8 ShowTime(u8 timeMode);             // 显示时间
void ShowMenu(void);                  // 显示添加用户菜单
void AddUserID(void);                 // 添加用户ID
u8 DeleteUser(void);                  // 删除用户
Boolean RemoveID(u8 *ID);             // 删除指定ID
Boolean RemoveAllID(void);            // 删除所有ID
char ReadCard(void);                  // 读卡

#endif
