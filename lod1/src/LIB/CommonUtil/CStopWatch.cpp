#include "pch.h"
#include "CStopWatch.h"

/*!
 * @brief コンストラクタ
*/
CStopWatch::CStopWatch()
{
    m_start = std::chrono::system_clock::now();
    m_stop = m_start;
    m_bRun = false;
}

/*!
 * @brief 計測開始
*/
void CStopWatch::Start()
{
    if (!m_bRun)
    {
        m_start = std::chrono::system_clock::now();
        m_bRun = true;
    }
}

/*!
 * @brief 計測終了
*/
void CStopWatch::Stop()
{
    if (m_bRun)
    {
        m_stop = std::chrono::system_clock::now();
        m_bRun = false;
    }
}

/*!
 * @brief 計測時間
 * @return 計測時間 msec
*/
double CStopWatch::GetMSec()
{
    double dMsec = 0;
    if (!m_bRun)
    {
        dMsec = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(m_stop - m_start).count());
    }
    return dMsec;
}

/*!
 * @brief 計測時間
 * @return 計測時間 sec
*/
double CStopWatch::GetSec()
{
    double dSec = 0;
    if (!m_bRun)
    {
        dSec = static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(m_stop - m_start).count());
    }
    return dSec;

}

/*!
 * @brief 計測時間
 * @param[out] nDay     日数
 * @param[out] nHour    時
 * @param[out] nMin     分
 * @param[out] nSec     秒
*/
void CStopWatch::GetTime(
    int &nDay, int &nHour, int &nMin, int &nSec)
{
    nDay = 0;
    nHour = 0;
    nMin = 0;
    nSec = 0;
    if (!m_bRun)
    {
        double dSec = GetSec();

        nDay = static_cast<int>(dSec / 3600.0 / 24.0);
        if (nDay > 0)
            dSec -= (3600.0 * 24.0) * nDay;
        nHour = static_cast<int>(dSec / 3600.0);

        if (nHour > 0)
            dSec -= 3600.0 * nHour;

        nMin = static_cast<int>(dSec / 60.0);
        nSec = static_cast<int>(dSec - 60.0 * nMin);
    }
}