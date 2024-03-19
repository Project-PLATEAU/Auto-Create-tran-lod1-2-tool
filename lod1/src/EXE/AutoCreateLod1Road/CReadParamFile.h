#pragma once
#include "CINIFileIO.h"

/*!
 * @brief 設定ファイル読み込みクラス
*/
class CReadParamFile
{
public:
    static CReadParamFile* GetInstance() { return &m_instance; }    // インスタンス取得
    bool Initialize(std::string strParamPath);  // 初期化
    bool IsErrCheckFromShp();

    int GetJPZone();                                    // 平面直角座標系の系番号
    std::string GetRoadSHPPath();                       // 道路縁SHPファイルパス
    std::string GetRoadFacilitiesPointSHPPath();        // 道路施設(点)SHPファイルパス
    std::string GetRoadFacilitiesLineSHPPath();         // 道路施設(線)SHPファイルパス
    std::string GetRoadFacilitiesPolygonSHPPath();      // 道路施設(面)SHPファイルパス
    std::string GetOutputFolderPath();                  // 出力フォルダパス
    std::string GetDMCodeAttribute();                   // DMコードの属性名
    std::string GetGeometryTypeAttribute();             // 図形区分コードの属性名
    double GetMinArea();                                // ポリゴンの面積の最小値
    double GetMaxDistance();                            // 車道交差部ポリゴン中心と交差点の距離の最大値
    std::string GetRoadBeforeDivisionShpFilePath();     // 分割前の道路ポリゴンのファイルパス(Debug)
    std::string GetDividedRoadShpFilePath();            // 分割後の道路ポリゴンのファイルパス(Debug)
    std::string GetIntersectionShpFilePath();           // 交差点情報のファイルパス(Debug)
    int GetRegionWidth();                               // 道路縁解析処理の注目領域幅(m)
    int GetRegionHeight();                              // 道路縁解析処理の注目領域高さ(m)
    int GetThreadNum();                                 // マルチスレッド数

protected:

private:
    CReadParamFile(void);                       // コンストラクタ
    virtual ~CReadParamFile(void);              // デストラクタ
    bool checkParam(void);                      // パラメータのエラーチェック

    static CReadParamFile m_instance;           // 自クラス唯一のインスタンス
    // 基本設定
    std::string m_strRoadSHPPath;                   // 道路縁SHPファイルパス
    std::string m_strRoadFacilitiesPointSHPPath;    // 道路施設(点)SHPファイルパス
    std::string m_strRoadFacilitiesLineSHPPath;     // 道路施設(線)SHPファイルパス
    std::string m_strRoadFacilitiesPolygonSHPPath;  // 道路施設(面)SHPファイルパス
    std::string m_strOutputFolderPath;              // 出力フォルダパス
    int m_nJPZone;                                  // 平面直角座標系の系番号
    int m_nRegionWidth;                             // 道路縁解析処理の注目領域幅(m)
    int m_nRegionHeight;                            // 道路縁解析処理の注目領域高さ(m)
    int m_nThreadNum;                               // マルチスレッド数

    // DMの属性設定
    std::string m_strDMCodeAttr;                // DMコードの属性名
    //std::string m_strRecordTypeAttr;            // レコードタイプの属性名
    std::string m_strGeometryTypeAttr;          // 図形区分コードの属性名

    // エラーチェックの設定
    std::string m_strMinArea;                           // ポリゴンの面積の最小値(入力確認用)
    double m_minArea;                                // ポリゴンの面積の最小値
    std::string m_strMaxDistance;                       // 車道交差部ポリゴン中心と交差点の距離の最大値(入力確認用)
    double m_maxDistance;                            // 車道交差部ポリゴン中心と交差点の距離の最大値
    std::string m_strRoadBeforeDivisionShpFilePath;     // 分割前の道路ポリゴンのファイルパス(Debug)
    std::string m_strDividedRoadShpFilePath;            // 分割後の道路ポリゴンのファイルパス(Debug)
    std::string m_strIntersectionShpFilePath;           // 交差点情報のファイルパス(Debug)
};

#define GetCreateParam() (CReadParamFile::GetInstance())
