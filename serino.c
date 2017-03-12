/**********************************************************************
* 版权所有 (C)2014, Zhou Zhaoxiong。
*
* 文件名称： SerialNo.c
* 文件标识： 无
* 内容摘要： 用于演示序列号的创建方法
* 其它说明： 无
* 当前版本： V1.0
* 作        者： 周兆熊
* 完成日期： 20140603
*
* 修改记录1：// 修改历史记录, 包括修改日期、版本号、修改人及修改内容
* 修改日期： 20140603
* 版 本 号： V1.0
* 修 改 人： Zhou Zhaoxiong
* 修改内容： 创建
**********************************************************************/

#include <afxinet.h>

// 数据类型
typedef unsigned char      UINT8;
typedef unsigned char      UINT16;
typedef unsigned int          UINT32;
typedef signed  int            INT32;


// 时间信息结构体
typedef struct
{
 UINT8  second;        /* 0-59 */
 UINT8  minute;        /* 0-59 */
 UINT8  hour;            /* 0-23 */
 UINT8  day;              /* 1-31 */
 UINT8  month;          /* 1-12 */
 UINT16  year;            /* 1994-2099 */
 UINT8  week;            /* 1-7 */
 UINT8  Count10ms;  /* 0-99 */
} ClockStruc;


// 函数声明
void  GetCurTime(ClockStruc *pCurrentTime);                          // 获取当前时间
INT32 CreateSerial(UINT8 *pSerialID, UINT32 iSerialSize);    // 创建序列号
INT32 main(void);                                                                            // 主函数


/**********************************************************************
* 功能描述： 获取当前时间                                            
* 输入参数： 无                                                      
* 输出参数： pCurrentTime-当前时间结构体                              
* 返 回 值： 无                                                      
* 其它说明： 无                                                      
* 修改日期      版本号      修改人          修改内容                  
* ----------------------------------------------------------------------------
* 20140603      V1.0    Zhou Zhaoxiong      创建                    
**********************************************************************/
void GetCurTime(ClockStruc *pCurrentTime)
{
    SYSTEMTIME tCurrentTime;

    GetLocalTime(&tCurrentTime);

    pCurrentTime->month    = (UINT8)tCurrentTime.wMonth;
    pCurrentTime->day      = (UINT8)tCurrentTime.wDay;
    pCurrentTime->hour      = (UINT8)tCurrentTime.wHour;
    pCurrentTime->minute    = (UINT8)tCurrentTime.wMinute;
    pCurrentTime->second    = (UINT8)tCurrentTime.wSecond;
    pCurrentTime->week      = (UINT8)tCurrentTime.wDayOfWeek;
    if (pCurrentTime->week == 0)        // 表示星期天
    {
        pCurrentTime->week = 7;
    }
}


/**********************************************************************
* 功能描述： 创建序列号 
* 输入参数： iSerialSize: 序列号长度
* 输出参数： pSerialID: 序列号
* 返 回 值： 0-成功  -1-失败
* 其它说明： 序列号的样式: MMDDHHMINSS_XXXXXX
* 修改日期          版本号          修改人        修改内容
* --------------------------------------------------------------
* 20140603          V1.0        Zhou Zhaoxiong      创建
***********************************************************************/
INT32 CreateSerial(UINT8 *pSerialID, UINT32 iSerialSize)
{
    ClockStruc    tClock  = {0};
    static UINT32 iTailNum = 0;
    
    if (NULL == pSerialID)
    {
        printf("CreateSerial: input parameter is NULL.\n");
        return -1;
    }

    GetCurTime(&tClock);


    _snprintf((char *)pSerialID, iSerialSize, "%02d%02d%02d%02d%02d_%06d", 
            tClock.month, tClock.day, tClock.hour, tClock.minute, tClock.second, iTailNum);


    iTailNum ++;
    if (iTailNum > 999999)
    {
        iTailNum = 0;
    }

    return 0;
}


/****************************************************************
* 功能描述:  主函数                                            
* 输入参数:  无                                                
* 输出参数:  无                                                
* 返 回 值： 0-执行成功  -1-执行失败                            
* 其他说明:  无                                                
* 修改日期        版本号        修改人        修改内容
* ------------------------------------------------------------------------
* 20140603        V1.0    Zhou Zhaoxiong    创建 
****************************************************************/
INT32 main(void)
{
    UINT32 iLoopFlag      = 0;      // 该变量用于表示循环标志
    INT32  iRetValue      = 0;        // 该变量用于表示调用CreateSerial函数返回的值
    UINT8  szSerialID[50] = {0};    // 该变量用于存放生成的序列号

    // 生成10个序列号, 并打印出来
    for (iLoopFlag = 0; iLoopFlag < 10; iLoopFlag ++)
    {
        iRetValue = CreateSerial(szSerialID, sizeof(szSerialID));
        if (iRetValue != 0)
        {
            printf("exec CreateSerial failed.\n");
            return -1;
        }

        printf("第%d个序列号为: %s\n", iLoopFlag + 1, szSerialID);
    }

    return 0;
}
