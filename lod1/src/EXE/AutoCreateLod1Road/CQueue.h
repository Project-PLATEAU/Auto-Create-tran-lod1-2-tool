#pragma once
#include <mutex>
#include <queue>


/*!
 * @brief 排他制御付きキュークラス
*/
template <typename T>
class CQueue
{
public:
    CQueue() {};    //!< コンストラクタ

    /*!
     * @brief コンストラクタ
    */
    CQueue(CQueue const &other)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_queue = other.m_queue;
    }

    CQueue &operator=(const CQueue&) = delete;  // 代入演算によるコピーの禁止
    ~CQueue() {};   //!< デストラクタ

    /*!
     * @brief push
     * @param[in] val 入力値
    */
    void push(T val)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(val);
    }

    /*!
     * @brief pop
     * @param[out] val  値
     * @return 処理結果
     * @retval true     取得成功
     * @retval false    取得失敗
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
     * @brief  空判定
     * @return 判定結果
     * @retval true     データなし
     * @retval false    データあり
    */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /*!
     * @brief データ数
     * @return データ数
    */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

protected:
private:
    mutable std::mutex m_mutex;     //!< 排他制御用
    std::queue<T> m_queue;          //!< queue
};

