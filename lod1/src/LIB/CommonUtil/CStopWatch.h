#pragma once
#include <chrono>

/*!
 * @brief �X�g�b�v�E�H�b�`�N���X
*/
class CStopWatch
{
public:
    CStopWatch();       // �R���X�g���N�^
    ~CStopWatch() {};   //!<�@�f�X�g���N�^

    void Start();       // �v���J�n
    void Stop();        // �v���I��

    double GetMSec();   // �v������ msec
    double GetSec();    // �v������ sec
    void GetTime(int &nDay, int &nHour, int &nMin, int &nSec);   // �v������

protected:

private:
    std::chrono::system_clock::time_point m_start;  //!< �J�n����
    std::chrono::system_clock::time_point m_stop;   //!< �I������
    bool m_bRun;    //!< �v���t���O
};

