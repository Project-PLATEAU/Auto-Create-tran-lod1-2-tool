#pragma once
#include <mutex>
#include <string>
#include <iostream>

class CLog
{
public:
    static CLog *GetInstance() { return &m_instance; } //!< インスタンス取得
    void ConsoleLog(const std::string &str);           // コンソール出力

protected:

private:
    CLog(void) {};          //!< コンストラクタ
    virtual ~CLog(void) {}; //!< デストラクタ

    static CLog m_instance; //!< 自クラス唯一のインスタンス
    std::mutex m_mutex;     //!< 排他制御用
};

#define GetLogger() (CLog::GetInstance())
