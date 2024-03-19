#pragma once
#include <mutex>
#include <queue>


/*!
 * @brief �r������t���L���[�N���X
*/
template <typename T>
class CQueue
{
public:
    CQueue() {};    //!< �R���X�g���N�^

    /*!
     * @brief �R���X�g���N�^
    */
    CQueue(CQueue const &other)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_queue = other.m_queue;
    }

    CQueue &operator=(const CQueue&) = delete;  // ������Z�ɂ��R�s�[�̋֎~
    ~CQueue() {};   //!< �f�X�g���N�^

    /*!
     * @brief push
     * @param[in] val ���͒l
    */
    void push(T val)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(val);
    }

    /*!
     * @brief pop
     * @param[out] val  �l
     * @return ��������
     * @retval true     �擾����
     * @retval false    �擾���s
    */
    bool pop(T &val)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
        {
            return false;
        }
        else
        {
            val = m_queue.front();
            m_queue.pop();
            return true;
        }
    }

    /*!
     * @brief  �󔻒�
     * @return ���茋��
     * @retval true     �f�[�^�Ȃ�
     * @retval false    �f�[�^����
    */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /*!
     * @brief �f�[�^��
     * @return �f�[�^��
    */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

protected:
private:
    mutable std::mutex m_mutex;     //!< �r������p
    std::queue<T> m_queue;          //!< queue
};

