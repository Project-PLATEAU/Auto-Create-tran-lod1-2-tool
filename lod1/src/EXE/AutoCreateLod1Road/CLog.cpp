#include "pch.h"
#include "CLog.h"

CLog CLog::m_instance;  // �C���X�^���X

/*!
 * @brief �R���\�[���o��
 * @param[in] str ������
 */
void CLog::ConsoleLog(const std::string &str)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << str << std::endl;
}
