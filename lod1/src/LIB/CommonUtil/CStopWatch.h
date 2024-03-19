#pragma once
#include <chrono>

/*!
 * @brief ストップウォッチクラス
*/
class CStopWatch
{
public:
    CStopWatch();       // コンストラクタ
    ~CStopWatch() {};   //!<　デストラクタ

    void Start();       // 計測開始
    void Stop();        // 計測終了

    double GetMSec();   // 計測時間 msec
    double GetSec();    // 計測時間 sec
    void GetTime(int &nDay, int &nHour, int &nMin, int &nSec);   // 計測時間

protected:

private:
    std::chrono::system_clock::time_point m_start;  //!< 開始時刻
    std::chrono::system_clock::time_point m_stop;   //!< 終了時刻
    bool m_bRun;    //!< 計測フラグ
};

