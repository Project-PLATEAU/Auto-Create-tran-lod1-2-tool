#pragma once

#include <boost/algorithm/clamp.hpp>

#include "CAnalyzeRoadEdgeDebugUtil.h"
#include "CTime.h"
#include "CRoadData.h"
#include "CEpsUtil.h"
#include "CCsvFileIO.h"
//
/// <summary>
/// �G���[�̎��
/// </summary>
enum class RoadModelErr
{
	SUCCESS								= 0,	// ����

	MISSING_MODEL_ERR					= 10,	// ���f���ʂ̌���

	TOPOLOGICAL_INVAILD_ERR				= 20,	// �g�|���W�[�s���i�s���|���S���j
	TOPOLOGICAL_SHORTAGE_POINT_ERR		= 21,	// �g�|���W�[�s���i���_�s���j
	TOPOLOGICAL_DUPLICATION_POINT_ERR	= 22,	// �g�|���W�[�s���i���_�d���j

	ANGLE_ERR							= 30,	// ���f���ʂ̕s�����p

	INTERSECTION_MISMATCH_ERR			= 40,	// �ԓ��������ƌ����_�̕s��v(��̃|���S���Ɍ����_���܂܂�Ȃ�)
	INTERSECTION_SAME_POINT_ERR			= 41,	// �ԓ��������ƌ����_�̕s��v(��̃|���S���ɏd�􂵂������̌����_)
	INTERSECTION_DIFFERENT_POINT_ERR	= 42,	// �ԓ��������ƌ����_�̕s��v(��̃|���S���ɕʂ̕����̌����_)

	EXCESS_ERR							= 50,	// ���H�|���S���̂͂ݏo��

	SUPERIMPOSE_ERR						= 60,	// �ԓ��������̏d��
	WITHIN_ERR							= 61,	// �ԓ��������̕��

	ROAD_DIVISION_ERR					= 70,	// �ԓ��������̓��H���������̕s��

	MINUSCULE_POLYGON_ERR				= 80,	// �ɏ��|���S��

	INTERSECTION_DISTANCE_ERR			= 90	// �ԓ��������|���S�����S�ƌ����_�ԋ���
};

/// <summary>
/// ���o���ꂽ�G���[�̃f�[�^�N���X
/// </summary>
class RoadErrInfo
{
public:
	/// <summary>
	/// �R���X�g���N�^
	/// </summary>
	RoadErrInfo() { Err = RoadModelErr::SUCCESS; }

	/// <summary>
	/// �R���X�g���N�^
	/// </summary>
	/// <param name="err">�G���[�̎��</param>
	/// <param name="point">�G���[���m�F���ꂽ���W</param>
	RoadErrInfo(RoadModelErr err, BoostPoint point)
		:Err(err), ErrPoint(point)
	{

	}

	/// <summary>
	/// �f�X�g���N�^
	/// </summary>
	~RoadErrInfo(){}

	/// <summary>
	/// �G���[�̎�ނ��擾����
	/// </summary>
	/// <returns>�G���[�̎��</returns>
	RoadModelErr GetErr() { return Err; }

	/// <summary>
	/// �G���[���m�F���ꂽ���W���擾����
	/// </summary>
	/// <returns>�G���[���m�F���ꂽ���W</returns>
	BoostPoint GetErrPoint() { return ErrPoint; }

private:
	/// <summary>
	/// �G���[�̎��
	/// </summary>
	RoadModelErr Err;

	/// <summary>
	/// �G���[���m�F���ꂽ���W
	/// </summary>
	BoostPoint ErrPoint;
};

/// <summary>
/// �쐬���ꂽ1�̓��H���f���̃f�[�^�N���X
/// </summary>
class CreatedRoadModelInfo
{
public:
	/// <summary>
	/// �R���X�g���N�^
	/// </summary>
	CreatedRoadModelInfo()
	{
		RoadData = CRoadData();
		RoadModelErrList = std::vector<RoadErrInfo>();
		IsAutoCreated = false;
	}

	/// <summary>
	/// �R���X�g���N�^
	/// </summary>
	/// <param name="data">���H�|���S��</param>
	/// <param name="isAutoCreated">�|���S�������̎����c�[���ɂ���č쐬���ꂽ�f�[�^���ǂ���</param>
	CreatedRoadModelInfo(CRoadData &data, bool isAutoCreated = true)
	{
		this->RoadData.Polygon(data.Polygon());
		this->RoadData.Type(data.Type());
		this->RoadData.Division(data.Division());
		this->RoadModelErrList = std::vector<RoadErrInfo>();
		IsAutoCreated = isAutoCreated;
	}

	/// <summary>
	/// �f�X�g���N�^
	/// </summary>
	~CreatedRoadModelInfo(){}

	/// <summary>
	/// ���H�����擾����
	/// </summary>
	/// <returns>���H�|���S��</returns>
	CRoadData GetRoadData() { return RoadData; }

	/// <summary>
	/// ���̓��H�|���S���Ō��o���ꂽ�G���[�̃��X�g���擾����
	/// </summary>
	/// <returns>�G���[�`�F�b�N����</returns>
	std::vector<RoadErrInfo> GetRoadModelErrList() { return RoadModelErrList; }

	/// <summary>
	/// �G���[�����X�g�ɒǉ�����
	/// </summary>
	/// <param name="err">���̓��H�|���S���Ō��o���ꂽ�G���[</param>
	void AddErr(RoadErrInfo err) { RoadModelErrList.emplace_back(err); }

	/// <summary>
	/// �G���[�`�F�b�N���Őݒ肳�ꂽ�|���S�����̌����_�����擾����
	/// ���H�|���S���������_�łȂ��ꍇ�͉����ۑ�����Ȃ�
	/// </summary>
	/// <returns>�G���[�`�F�b�N���Őݒ肳�ꂽ�|���S�����̌����_���</returns>
	BoostPoint GetIntersectionPoint() { return IntersectionPoint; }

	/// <summary>
	/// �G���[�`�F�b�N���Őݒ肳�ꂽ�|���S�����̌����_����ݒ肷��
	/// </summary>
	/// <param name="point">�G���[�`�F�b�N���Őݒ肳�ꂽ�|���S�����̌����_���</param>
	void SetIntersectionPoint(BoostPoint point) { IntersectionPoint = point; }

private:
	/// <summary>
	/// ���H�|���S��
	/// </summary>
	CRoadData RoadData;

	/// <summary>
	/// �G���[�`�F�b�N����
	/// </summary>
	std::vector<RoadErrInfo> RoadModelErrList;

	/// <summary>
	/// �|���S�������������c�[���ɂ���Ď����Œǉ����ꂽ���̂�
	/// </summary>
	bool IsAutoCreated;

	/// <summary>
	/// �G���[�`�F�b�N���Őݒ肳�ꂽ�|���S�����̌����_���
	/// </summary>
	BoostPoint IntersectionPoint;
};

/// <summary>
/// �G���[�`�F�b�N�ɕK�v�ȃf�[�^�N���X
/// </summary>
class RoadModelData
{
public:
	/// <summary>
	/// �R���X�g���N�^
	/// </summary>
	RoadModelData(){}

	/// <summary>
	/// �R���X�g���N�^
	/// </summary>
	/// <param name="roadPolygonList">�����c�[���Ő������ꂽ���H�|���S�����</param>
	/// <param name="preRoadPolygonList">�|���S�������O�̓��H�|���S���̔z��</param>
	/// <param name="intersectionPointList">�����_�|�C���g�̔z��</param>
	RoadModelData(std::vector<CreatedRoadModelInfo> roadPolygonList, BoostMultiPolygon preRoadPolygonList, BoostMultiPoints intersectionPointList)
		: RoadPolygonList(roadPolygonList), PreRoadPolygonList(preRoadPolygonList), IntersectionPointList(intersectionPointList)
	{

	}

	/// <summary>
	/// �f�X�g���N�^
	/// </summary>
	~RoadModelData(){}

	/// <summary>
	/// �����c�[���Ő������ꂽ���H�|���S�����̔z����擾����
	/// </summary>
	/// <returns>�����c�[���Ő������ꂽ���H�|���S�����</returns>
	std::vector<CreatedRoadModelInfo>& GetRoadPolygonList() { return RoadPolygonList; }

	/// <summary>
	/// �����c�[���Ő������ꂽ���H�|���S�����̔z��̓�
	/// �|���S���݂̂�BoostMultiPolygon�^�Ŏ擾����
	/// </summary>
	/// <returns>�����c�[���Ő������ꂽ���H�|���S�����̃|���S���z��</returns>
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
	/// �|���S�������O�̓��H�|���S���̔z����擾����
	/// </summary>
	/// <returns>�|���S�������O�̓��H�|���S���̔z��</returns>
	BoostMultiPolygon& GetPreRoadPolygonList() { return PreRoadPolygonList; }

	/// <summary>
	/// �����_�|�C���g�̔z����擾����
	/// </summary>
	/// <returns>�����_�|�C���g�̔z��</returns>
	BoostMultiPoints& GetIntersectionPointList() { return IntersectionPointList; }

	/// <summary>
	/// ���o�����G���[�ɑΉ�����|���S�����Ȃ��G���[��ǉ�����
	/// </summary>
	/// <param name="err">���̓��H�|���S���Ō��o���ꂽ�G���[</param>
	void AddErrWithEmptyPolygon(RoadErrInfo err)
	{
		CRoadData road;
		CreatedRoadModelInfo info(road, false);
		info.AddErr(err);

		RoadPolygonList.emplace_back(info);
	}

private:
	/// <summary>
	/// �����c�[���Ő������ꂽ���H�|���S�����
	/// �G���[�o�͗p�ɋ�̃|���S�����܂ޏ����܂�
	/// </summary>
	std::vector<CreatedRoadModelInfo> RoadPolygonList;

	/// <summary>
	/// �|���S�������O�̓��H�|���S���̔z��
	/// </summary>
	BoostMultiPolygon PreRoadPolygonList;

	/// <summary>
	/// �����_�|�C���g�̔z��
	/// </summary>
	BoostMultiPoints IntersectionPointList;

};

/// <summary>
/// �쐬�������f���ɃG���[���Ȃ����m�F����N���X
/// </summary>
class RoadModelErrorChecker
{
public:
	/// <summary>
	/// �R���X�g���N�^
	/// </summary>
	RoadModelErrorChecker();

	/// <summary>
	/// �f�X�g���N�^
	/// </summary>
	~RoadModelErrorChecker();

	/// <summary>
	/// �G���[�`�F�b�N���s(�_�~�[�f�[�^���g�p�����f�o�b�O�p)
	/// </summary>
	/// <param name="minArea">�ɏ��|���S����臒l</param>
	/// <param name="maxDistance">�ԓ��������|���S�����S�ƌ����_�ԋ�����臒l</param>
	static void TestRun(double minArea = 1.0, double maxDistance = 5.0);

	/// <summary>
	/// �G���[�`�F�b�N���s(Shp�t�@�C�����g�p�����f�o�b�O�p)
	/// </summary>
	/// <param name="intputShpFilePath">���͂Ɏg�p����Shp�t�@�C��</param>
	/// <param name="outputErrFilePath">�G���[�`�F�b�N�̌��ʂ��o�͂���csv�t�@�C���p�X</param>
	/// <param name="minArea">�ɏ��|���S����臒l</param>
	/// <param name="maxDistance">�ԓ��������|���S�����S�ƌ����_�ԋ�����臒l</param>
	static void RunFromSHP(std::string inputRoadBeforeDivisionShpFilePath, std::string inputDividedShpFilePath, std::string inputIntersectionShpFilePath, std::string outputErrFilePath, double minArea = 1.0, double maxDistance = 5.0);

	/// <summary>
	/// �G���[�`�F�b�N���s
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	/// <param name="outputErrFilePath">�G���[�`�F�b�N�̌��ʂ��o�͂���csv�t�@�C���p�X</param>
    /// <param name="isNewFile">csv�t�@�C����V�K�쐬���邩�ۂ�</param>
	/// <param name="minArea">�ɏ��|���S����臒l</param>
    /// <param name="maxDistance">�ԓ��������|���S�����S�ƌ����_�ԋ�����臒l</param>
	static void Run(RoadModelData& data, std::string outputErrFilePath, bool isNewFile = true, double minArea = 1.0, double maxDistance = 5.0);


    /// <summary>
    /// �G���[�`�F�b�N���s
    /// </summary>
    /// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
    /// <param name="minArea">�ɏ��|���S����臒l</param>
    /// <param name="maxDistance">�ԓ��������|���S�����S�ƌ����_�ԋ�����臒l</param>
    /// <returns>�G���[���b�Z�[�W</returns>
    ///
    static std::vector<std::vector<std::string>> Run(RoadModelData &data, double minArea = 1.0, double maxDistance = 5.0);

    /// <summary>
    /// �G���[�`�F�b�N���ʂ̃t�@�C���o��
    /// </summary>
    /// <param name="errMsg">�G���[���b�Z�[�W</param>
    /// <param name="outputErrFilePath">�G���[�`�F�b�N�̌��ʂ��o�͂���csv�t�@�C���p�X</param>
    static void SaveErr(std::vector<std::vector<std::string>> &errMsg, std::string outputErrFilePath);

private:
	/// <summary>
	/// ���f���ʌ����m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	static void CheckMissingModelErr(RoadModelData& data);

	/// <summary>
	/// �g�|���W�[�s���m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	static void CheckTopologicalErr(RoadModelData& data);

	/// <summary>
	/// ���f���ʂ̓��p�m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	static void CheckAngleErr(RoadModelData& data);

	/// <summary>
	/// �ԓ��������ƌ����_�̈�v�m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	static void CheckIntersectionErr(RoadModelData& data);

	/// <summary>
	/// ���H�|���S���݂̂͂����m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	static void CheckExcessErr(RoadModelData& data);

	/// <summary>
	/// �ԓ��������̏d��m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	static void CheckSuperimposeErr(RoadModelData& data);

	/// <summary>
	/// �����������̓��H���������̊m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	static void CheckRoadDivisionErr(RoadModelData& data);

	/// <summary>
	/// �ɏ��|���S���̊m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	/// <param name="minArea">�ɏ��|���S����臒l</param>
	static void CheckMinusculePolygonErr(RoadModelData& data, double minArea);

	/// <summary>
	/// �ԓ��������|���S�����S�ƌ����_�ԋ����̊m�F
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	/// <param name="maxDistance">�ԓ��������|���S�����S�ƌ����_�ԋ�����臒l</param>
	static void CheckIntersectionDistanceErr(RoadModelData& data, double maxDistance);

	/// <summary>
	/// �G���[�`�F�b�N���ʂ̃t�@�C���o��
	/// </summary>
	/// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
	/// <param name="outputErrFilePath">�G���[�`�F�b�N�̌��ʂ��o�͂���csv�t�@�C���p�X</param>
    /// <param name="isNewFile">csv�t�@�C����V�K�쐬���邩�ۂ�</param>
	static void SaveErr(RoadModelData& data, std::string outputErrFilePath, bool isNewFile);

	/// <summary>
	/// �Ώۂ̓��H���f���̃f�[�^�������Ώۂł��邩�ǂ���
	/// </summary>
	/// <param name="data">�쐬���ꂽ1�̓��H���f���̃f�[�^�N���X</param>
	/// <returns>�|���S���s�����Ȃ������ΏۂȂ�true</returns>
	/// <returns>��L�ȊO�Ȃ�false</returns>
	static bool IsInspectionInfo(CreatedRoadModelInfo createdRoadModelInfo);

	/// <summary>
	/// �p�x�����߂�
	/// </summary>
	/// <param name="o">�p�x�����߂钆�S�̒��_</param>
	/// <param name="u">o�Ɏn���x�N�g�������Z�������_</param>
	/// <param name="v">o�ɓ��a�x�N�g�������Z�������_</param>
	/// <returns>�puov�̒l</returns>
	static double Angle(BoostPoint o, BoostPoint u, BoostPoint v);

	/// <summary>
	/// ��̃|���S���̂��ׂĂ̊p�x�����߂�
	/// </summary>
	/// <param name="o">�p�x�����߂�|���S��</param>
	/// <returns>�|���S�����̊p�x�̔z��</returns>
	static std::vector<double> Angles(BoostPolygon polygon);

	/// <summary>
	/// ��̃|���S���̒��S�_�����߂�
	/// </summary>
	/// <param name="o">���S�_�����߂�|���S��</param>
	/// <returns>�|���S���̒��S�_</returns>
	static BoostPoint GetPolygonPoint(BoostPolygon polygon);

    /// <summary>
    /// �G���[���b�Z�[�W�쐬
    /// </summary>
    /// <param name="data">�G���[�`�F�b�N�ɕK�v�ȃf�[�^</param>
    /// <param name="isTitle">�^�C�g���s�̕t�^�t���O</param>
    /// <returns>�G���[���b�Z�[�W</returns>
    static std::vector<std::vector<std::string>> CreateErrMsg(RoadModelData &data);

};