#include "pch.h"
#include "CStopWatch.h"

/*!
 * @brief �R���X�g���N�^
*/
CStopWatch::CStopWatch()
{
    m_start = std::chrono::system_clock::now();
    m_stop = m_start;
    m_bRun = false;
}

/*!
 * @brief �v���J�n
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
 * @brief �v���I��
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
 * @brief �v������
 * @return �v������ msec
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
 * @brief �v������
 * @return �v������ sec
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
 * @brief �v������
 * @param[out] nDay     ����
 * @param[out] nHour    ��
 * @param[out] nMin     ��
 * @param[out] nSec     �b
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