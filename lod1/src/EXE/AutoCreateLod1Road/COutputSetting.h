#pragma once
#include <string>
#include <filesystem>
#include "CFileUtil.h"

/*!
 * @brief 出力ファイル設定クラス
*/
class COutputSetting
{
public:
    static COutputSetting *GetInstance() { return &m_instance; }    // インスタンス取得

    // 初期化
    bool Initialize(
        std::string strOutputFolderPath,
        std::string strShpFileName,
        std::string strErrFileName);

    /*!
     * @brief フォルダ作成
     * @param strPath [in] フォルダパス
     * @return 処理結果
     * @retval true     作成成功 or 既存フォルダ
     * @retval false    作成失敗
     */
    static bool CreateFolder(std::string strPath)
    {
        bool bRet = CFileUtil::IsExistPath(strPath);
        if (!bRet)
        {
            // 存在しない場合
            bRet = std::filesystem::create_directories(strPath);
        }
        return bRet;
    }

    std::string GetShpFilePath(void);           // shpファイルパス(穴無しポリゴン)
    std::string GetShpFilePathWithHoles(void);  // shpファイルパス(穴有りポリゴン)
    std::string GetErrFilePath(void);           // エラーチェック結果のファイルパス

protected:

private:

    COutputSetting(void);                   // コンストラクタ
    virtual ~COutputSetting(void);          // デストラクタ

    static COutputSetting m_instance;       //!< 自クラス唯一のインスタンス
    std::string m_strShpFilePath;           //!< shpファイルパス(穴無しポリゴン)
    std::string m_strShpFilePathWithHoles;  //!< shpファイルパス(穴有りポリゴン)
    std::string m_strErrFilePath;           //!< エラーチェック結果のファイルパス

};

#define GetOutputSetting() (COutputSetting::GetInstance())
