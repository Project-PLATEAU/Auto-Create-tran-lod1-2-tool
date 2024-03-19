#pragma once
#include <mutex>
#include <string>
#include <iostream>

class CLog
{
public:
    static CLog *GetInstance() { return &m_instance; } //!< �C���X�^���X�擾
    void ConsoleLog(const std::string &str);           // �R���\�[���o��

protected:

private:
    CLog(void) {};          //!< �R���X�g���N�^
    virtual ~CLog(void) {}; //!< �f�X�g���N�^

    static CLog m_instance; //!< ���N���X�B��̃C���X�^���X
    std::mutex m_mutex;     //!< �r������p
};

#define GetLogger() (CLog::GetInstance())
