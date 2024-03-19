#include "pch.h"
#include "CLog.h"

CLog CLog::m_instance;  // インスタンス

/*!
 * @brief コンソール出力
 * @param[in] str 文字列
 */
void CLog::ConsoleLog(const std::string &str)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << str << std::endl;
}
