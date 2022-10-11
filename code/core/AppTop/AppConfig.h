#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

//与CTP API交互时各种等待时间的选择
#define LOGIN_WAIT_TIME         (30000)   //20秒，等待登录的最长时间
#define REQ_WAIT_TIME           (10000)   //10秒，等待查询结果的最长时间
#define MD_WAIT_TIME            (10000)   //10秒，等待市场行情的最长时间
#define REQ_INTERVAL            (2000)    //2秒，查询一次睡眠时间(CTP 1秒钟只允许1次查询)
#define LEAST_TICK_INTERVAL     (0.01)     //相应Tick数据间最小时间间隔， 取0.01秒，时间取短一些，以求尽快响应
#define REFRESH_INTERNAL        (6)       //6秒刷新一次投资者持仓信息和账户信息
#define UPDATE_STAT_INTERVAL    (1.0)     //更新统计数据的时间间隔，1秒
#define WAIT_THREAD_FINISH      (30000)   //30秒, 等待线程结束的时间

//线程给出活动信息的时间间隔（秒数）
#define THREAD_ACTIVE_INDIC     (3)
//计算T型报价各种数学参数(如希腊值)的更新时间间隔（秒数)
#define TSHAPE_CAL_INTERVAL     (3)

//是否在DbgView中显示开平仓策略扫描的调试信息
//#define SCAN_DBGVIEW_DISP

//配置行情数据所用的参数
#define GET_KLINE_NUM           (50)      //最多获取50日K线数据
#define HV_USED_DAYS            (20)      //计算历史波动率用到的天数

//各种文件配置
#define MANUAL_TRADE_CONFIG           "ManualTradeConfig.txt"
#define STRATEGY_OUTFILE              "strategy_out.txt"
#define INVESTOR_POSITION_FILE        "InvestorPosition.txt"
#define PORTFOLIO_POSITION_FILE       "PortfolioPosition.txt"
#define EXPIRE_CHECK_FILE             "ExpireCheck.txt"

//调试日志文件
//#define DEBUG_LOG_FILE                "./output/DebugLog.txt"

#define YEAR_TRADE_DAY_RATIO     (240.0/365.0)   //一年中交易日所占的比例
#define STAT_SINGLE_SIDE_LIMIT   (10000)  //套利统计时，单边统计区间数目的最大限制    

//交易信息刷新存入数据库的时间间隔 (30秒）
#define DB_SAVE_INTERVAL                 (30)
//报单确认的窗口长度
#define ORDER_ACK_WIND                   (700)
//成交确认的窗口长度
#define TRADE_ACK_WIND                   (100)
//交易最后一天下午1点STOPOPEN_MINUTES分钟之后, 停止开仓交易
#define STOPOPEN_MINUTES                 (5.0)
//开仓时直接以对手价下单相对于钓鱼单开仓的盈利减少量系数(以组合最小价差为单位)
#define NOFISH_DEC_COEF                  (2.0)   
//证券锁定/解锁所等待的最大时间(毫秒)
#define MAX_LOCK_WAIT_TIME               (500)

//欲交易标的的最小持仓量
#define MIN_UNDER_OPENINTERESTS          (20000)  
//欲交易期权的最小持仓量
#define MIN_OPTION_OPENINTERESTS         (1000)  
//欲交易标的的最小成交量
#define MIN_UNDER_VOLUME                 (20)       
//欲交易期权的最小的成交量
#define MIN_OPTION_VOLUME                (2)     

//股票期权中的一些宏定义期货期权中可能没有，反之亦然
#ifdef FUTURE_OPTION_ONLY 

#ifndef THOST_FTDC_IS_VolatilityInterrupt
#define THOST_FTDC_IS_VolatilityInterrupt '9'
#endif

#endif  //FUTURE_OPTION_ONLY

#endif
