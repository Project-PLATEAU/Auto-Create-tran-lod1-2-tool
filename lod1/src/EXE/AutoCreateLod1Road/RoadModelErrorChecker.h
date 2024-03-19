#pragma once

#include <boost/algorithm/clamp.hpp>

#include "CAnalyzeRoadEdgeDebugUtil.h"
#include "CTime.h"
#include "CRoadData.h"
#include "CEpsUtil.h"
#include "CCsvFileIO.h"
//
/// <summary>
/// エラーの種類
/// </summary>
enum class RoadModelErr
{
	SUCCESS								= 0,	// 成功

	MISSING_MODEL_ERR					= 10,	// モデル面の欠落

	TOPOLOGICAL_INVAILD_ERR				= 20,	// トポロジー不正（不正ポリゴン）
	TOPOLOGICAL_SHORTAGE_POINT_ERR		= 21,	// トポロジー不正（頂点不足）
	TOPOLOGICAL_DUPLICATION_POINT_ERR	= 22,	// トポロジー不正（頂点重複）

	ANGLE_ERR							= 30,	// モデル面の不正内角

	INTERSECTION_MISMATCH_ERR			= 40,	// 車道交差部と交差点の不一致(一つのポリゴンに交差点が含まれない)
	INTERSECTION_SAME_POINT_ERR			= 41,	// 車道交差部と交差点の不一致(一つのポリゴンに重畳した複数の交差点)
	INTERSECTION_DIFFERENT_POINT_ERR	= 42,	// 車道交差部と交差点の不一致(一つのポリゴンに別の複数の交差点)

	EXCESS_ERR							= 50,	// 道路ポリゴンのはみ出し

	SUPERIMPOSE_ERR						= 60,	// 車道交差部の重畳
	WITHIN_ERR							= 61,	// 車道交差部の包含

	ROAD_DIVISION_ERR					= 70,	// 車道交差部の道路分割線数の不正

	MINUSCULE_POLYGON_ERR				= 80,	// 極小ポリゴン

	INTERSECTION_DISTANCE_ERR			= 90	// 車道交差部ポリゴン中心と交差点間距離
};

/// <summary>
/// 検出されたエラーのデータクラス
/// </summary>
class RoadErrInfo
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	RoadErrInfo() { Err = RoadModelErr::SUCCESS; }

	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="err">エラーの種類</param>
	/// <param name="point">エラーが確認された座標</param>
	RoadErrInfo(RoadModelErr err, BoostPoint point)
		:Err(err), ErrPoint(point)
	{

	}

	/// <summary>
	/// デストラクタ
	/// </summary>
	~RoadErrInfo(){}

	/// <summary>
	/// エラーの種類を取得する
	/// </summary>
	/// <returns>エラーの種類</returns>
	RoadModelErr GetErr() { return Err; }

	/// <summary>
	/// エラーが確認された座標を取得する
	/// </summary>
	/// <returns>エラーが確認された座標</returns>
	BoostPoint GetErrPoint() { return ErrPoint; }

private:
	/// <summary>
	/// エラーの種類
	/// </summary>
	RoadModelErr Err;

	/// <summary>
	/// エラーが確認された座標
	/// </summary>
	BoostPoint ErrPoint;
};

/// <summary>
/// 作成された1つの道路モデルのデータクラス
/// </summary>
class CreatedRoadModelInfo
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CreatedRoadModelInfo()
	{
		RoadData = CRoadData();
		RoadModelErrList = std::vector<RoadErrInfo>();
		IsAutoCreated = false;
	}

	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="data">道路ポリゴン</param>
	/// <param name="isAutoCreated">ポリゴン分割の自動ツールによって作成されたデータかどうか</param>
	CreatedRoadModelInfo(CRoadData &data, bool isAutoCreated = true)
	{
		this->RoadData.Polygon(data.Polygon());
		this->RoadData.Type(data.Type());
		this->RoadData.Division(data.Division());
		this->RoadModelErrList = std::vector<RoadErrInfo>();
		IsAutoCreated = isAutoCreated;
	}

	/// <summary>
	/// デストラクタ
	/// </summary>
	~CreatedRoadModelInfo(){}

	/// <summary>
	/// 道路情報を取得する
	/// </summary>
	/// <returns>道路ポリゴン</returns>
	CRoadData GetRoadData() { return RoadData; }

	/// <summary>
	/// この道路ポリゴンで検出されたエラーのリストを取得する
	/// </summary>
	/// <returns>エラーチェック結果</returns>
	std::vector<RoadErrInfo> GetRoadModelErrList() { return RoadModelErrList; }

	/// <summary>
	/// エラーをリストに追加する
	/// </summary>
	/// <param name="err">この道路ポリゴンで検出されたエラー</param>
	void AddErr(RoadErrInfo err) { RoadModelErrList.emplace_back(err); }

	/// <summary>
	/// エラーチェック内で設定されたポリゴン内の交差点情報を取得する
	/// 道路ポリゴンが交差点でない場合は何も保存されない
	/// </summary>
	/// <returns>エラーチェック内で設定されたポリゴン内の交差点情報</returns>
	BoostPoint GetIntersectionPoint() { return IntersectionPoint; }

	/// <summary>
	/// エラーチェック内で設定されたポリゴン内の交差点情報を設定する
	/// </summary>
	/// <param name="point">エラーチェック内で設定されたポリゴン内の交差点情報</param>
	void SetIntersectionPoint(BoostPoint point) { IntersectionPoint = point; }

private:
	/// <summary>
	/// 道路ポリゴン
	/// </summary>
	CRoadData RoadData;

	/// <summary>
	/// エラーチェック結果
	/// </summary>
	std::vector<RoadErrInfo> RoadModelErrList;

	/// <summary>
	/// ポリゴン分割が自動ツールによって自動で追加されたものか
	/// </summary>
	bool IsAutoCreated;

	/// <summary>
	/// エラーチェック内で設定されたポリゴン内の交差点情報
	/// </summary>
	BoostPoint IntersectionPoint;
};

/// <summary>
/// エラーチェックに必要なデータクラス
/// </summary>
class RoadModelData
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	RoadModelData(){}

	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="roadPolygonList">自動ツールで生成された道路ポリゴン情報</param>
	/// <param name="preRoadPolygonList">ポリゴン分割前の道路ポリゴンの配列</param>
	/// <param name="intersectionPointList">交差点ポイントの配列</param>
	RoadModelData(std::vector<CreatedRoadModelInfo> roadPolygonList, BoostMultiPolygon preRoadPolygonList, BoostMultiPoints intersectionPointList)
		: RoadPolygonList(roadPolygonList), PreRoadPolygonList(preRoadPolygonList), IntersectionPointList(intersectionPointList)
	{

	}

	/// <summary>
	/// デストラクタ
	/// </summary>
	~RoadModelData(){}

	/// <summary>
	/// 自動ツールで生成された道路ポリゴン情報の配列を取得する
	/// </summary>
	/// <returns>自動ツールで生成された道路ポリゴン情報</returns>
	std::vector<CreatedRoadModelInfo>& GetRoadPolygonList() { return RoadPolygonList; }

	/// <summary>
	/// 自動ツールで生成された道路ポリゴン情報の配列の内
	/// ポリゴンのみをBoostMultiPolygon型で取得する
	/// </summary>
	/// <returns>自動ツールで生成された道路ポリゴン情報のポリゴン配列</returns>
	BoostMultiPolygon GetRoadBoostMultiPolygon()
	{
		BoostMultiPolygon multiPolygon;
		for (CreatedRoadModelInfo& info : RoadPolygonList)
		{
			multiPolygon.emplace_back(info.GetRoadData().Polygon());
		}
		return multiPolygon;
	}

	/// <summary>
	/// ポリゴン分割前の道路ポリゴンの配列を取得する
	/// </summary>
	/// <returns>ポリゴン分割前の道路ポリゴンの配列</returns>
	BoostMultiPolygon& GetPreRoadPolygonList() { return PreRoadPolygonList; }

	/// <summary>
	/// 交差点ポイントの配列を取得する
	/// </summary>
	/// <returns>交差点ポイントの配列</returns>
	BoostMultiPoints& GetIntersectionPointList() { return IntersectionPointList; }

	/// <summary>
	/// 検出したエラーに対応するポリゴンがないエラーを追加する
	/// </summary>
	/// <param name="err">この道路ポリゴンで検出されたエラー</param>
	void AddErrWithEmptyPolygon(RoadErrInfo err)
	{
		CRoadData road;
		CreatedRoadModelInfo info(road, false);
		info.AddErr(err);

		RoadPolygonList.emplace_back(info);
	}

private:
	/// <summary>
	/// 自動ツールで生成された道路ポリゴン情報
	/// エラー出力用に空のポリゴンを含む情報も含む
	/// </summary>
	std::vector<CreatedRoadModelInfo> RoadPolygonList;

	/// <summary>
	/// ポリゴン分割前の道路ポリゴンの配列
	/// </summary>
	BoostMultiPolygon PreRoadPolygonList;

	/// <summary>
	/// 交差点ポイントの配列
	/// </summary>
	BoostMultiPoints IntersectionPointList;

};

/// <summary>
/// 作成したモデルにエラーがないか確認するクラス
/// </summary>
class RoadModelErrorChecker
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	RoadModelErrorChecker();

	/// <summary>
	/// デストラクタ
	/// </summary>
	~RoadModelErrorChecker();

	/// <summary>
	/// エラーチェック実行(ダミーデータを使用したデバッグ用)
	/// </summary>
	/// <param name="minArea">極小ポリゴンの閾値</param>
	/// <param name="maxDistance">車道交差部ポリゴン中心と交差点間距離の閾値</param>
	static void TestRun(double minArea = 1.0, double maxDistance = 5.0);

	/// <summary>
	/// エラーチェック実行(Shpファイルを使用したデバッグ用)
	/// </summary>
	/// <param name="intputShpFilePath">入力に使用するShpファイル</param>
	/// <param name="outputErrFilePath">エラーチェックの結果を出力するcsvファイルパス</param>
	/// <param name="minArea">極小ポリゴンの閾値</param>
	/// <param name="maxDistance">車道交差部ポリゴン中心と交差点間距離の閾値</param>
	static void RunFromSHP(std::string inputRoadBeforeDivisionShpFilePath, std::string inputDividedShpFilePath, std::string inputIntersectionShpFilePath, std::string outputErrFilePath, double minArea = 1.0, double maxDistance = 5.0);

	/// <summary>
	/// エラーチェック実行
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	/// <param name="outputErrFilePath">エラーチェックの結果を出力するcsvファイルパス</param>
    /// <param name="isNewFile">csvファイルを新規作成するか否か</param>
	/// <param name="minArea">極小ポリゴンの閾値</param>
    /// <param name="maxDistance">車道交差部ポリゴン中心と交差点間距離の閾値</param>
	static void Run(RoadModelData& data, std::string outputErrFilePath, bool isNewFile = true, double minArea = 1.0, double maxDistance = 5.0);


    /// <summary>
    /// エラーチェック実行
    /// </summary>
    /// <param name="data">エラーチェックに必要なデータ</param>
    /// <param name="minArea">極小ポリゴンの閾値</param>
    /// <param name="maxDistance">車道交差部ポリゴン中心と交差点間距離の閾値</param>
    /// <returns>エラーメッセージ</returns>
    ///
    static std::vector<std::vector<std::string>> Run(RoadModelData &data, double minArea = 1.0, double maxDistance = 5.0);

    /// <summary>
    /// エラーチェック結果のファイル出力
    /// </summary>
    /// <param name="errMsg">エラーメッセージ</param>
    /// <param name="outputErrFilePath">エラーチェックの結果を出力するcsvファイルパス</param>
    static void SaveErr(std::vector<std::vector<std::string>> &errMsg, std::string outputErrFilePath);

private:
	/// <summary>
	/// モデル面欠落確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	static void CheckMissingModelErr(RoadModelData& data);

	/// <summary>
	/// トポロジー不正確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	static void CheckTopologicalErr(RoadModelData& data);

	/// <summary>
	/// モデル面の内角確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	static void CheckAngleErr(RoadModelData& data);

	/// <summary>
	/// 車道交差部と交差点の一致確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	static void CheckIntersectionErr(RoadModelData& data);

	/// <summary>
	/// 道路ポリゴンのはみだし確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	static void CheckExcessErr(RoadModelData& data);

	/// <summary>
	/// 車道交差部の重畳確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	static void CheckSuperimposeErr(RoadModelData& data);

	/// <summary>
	/// 書道交差部の道路分割線数の確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	static void CheckRoadDivisionErr(RoadModelData& data);

	/// <summary>
	/// 極小ポリゴンの確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	/// <param name="minArea">極小ポリゴンの閾値</param>
	static void CheckMinusculePolygonErr(RoadModelData& data, double minArea);

	/// <summary>
	/// 車道交差部ポリゴン中心と交差点間距離の確認
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	/// <param name="maxDistance">車道交差部ポリゴン中心と交差点間距離の閾値</param>
	static void CheckIntersectionDistanceErr(RoadModelData& data, double maxDistance);

	/// <summary>
	/// エラーチェック結果のファイル出力
	/// </summary>
	/// <param name="data">エラーチェックに必要なデータ</param>
	/// <param name="outputErrFilePath">エラーチェックの結果を出力するcsvファイルパス</param>
    /// <param name="isNewFile">csvファイルを新規作成するか否か</param>
	static void SaveErr(RoadModelData& data, std::string outputErrFilePath, bool isNewFile);

	/// <summary>
	/// 対象の道路モデルのデータが検査対象であるかどうか
	/// </summary>
	/// <param name="data">作成された1つの道路モデルのデータクラス</param>
	/// <returns>ポリゴン不正がなく検査対象ならtrue</returns>
	/// <returns>上記以外ならfalse</returns>
	static bool IsInspectionInfo(CreatedRoadModelInfo createdRoadModelInfo);

	/// <summary>
	/// 角度を求める
	/// </summary>
	/// <param name="o">角度を求める中心の頂点</param>
	/// <param name="u">oに始線ベクトルを加算した頂点</param>
	/// <param name="v">oに動径ベクトルを加算した頂点</param>
	/// <returns>角uovの値</returns>
	static double Angle(BoostPoint o, BoostPoint u, BoostPoint v);

	/// <summary>
	/// 一つのポリゴンのすべての角度を求める
	/// </summary>
	/// <param name="o">角度を求めるポリゴン</param>
	/// <returns>ポリゴン内の角度の配列</returns>
	static std::vector<double> Angles(BoostPolygon polygon);

	/// <summary>
	/// 一つのポリゴンの中心点を求める
	/// </summary>
	/// <param name="o">中心点を求めるポリゴン</param>
	/// <returns>ポリゴンの中心点</returns>
	static BoostPoint GetPolygonPoint(BoostPolygon polygon);

    /// <summary>
    /// エラーメッセージ作成
    /// </summary>
    /// <param name="data">エラーチェックに必要なデータ</param>
    /// <param name="isTitle">タイトル行の付与フラグ</param>
    /// <returns>エラーメッセージ</returns>
    static std::vector<std::vector<std::string>> CreateErrMsg(RoadModelData &data);

};