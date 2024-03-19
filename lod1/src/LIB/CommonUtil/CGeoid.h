#pragma once

#include <cstdio>

//=============================================================================
// class CGeoidData
//=============================================================================
class CGeoidData
{
public:
	double Value;
	short Flag;

	CGeoidData() { Value = 0.0; Flag = 0; }
};

//=============================================================================
// class CGeoid
//=============================================================================
class CGeoid
{
	int GeoidType;
	wchar_t GeoidPath[1024];

	double OriginB;
	double OriginL;

	int Width;    // �ǂݍ��񂾃��b�V������
	int Height;   // �ǂݍ��񂾃��b�V���c��

	CGeoidData** Data;

	double MinB;  // ��[�̈ܓx[deg]
	double MinL;  // ���[�̌o�x[deg]
	double MaxB;  // �k�[�̈ܓx[deg]
	double MaxL;  // ���[�̌o�x[deg]
	double DB;    // �ܓx��[deg]
	double DL;    // �o�x��[deg]

	double Exception;

	bool CrossDateLine;  // EGM�̂Ƃ��A���t�ύX��(E180 = W180)���܂����œǂݍ���ł��邩�ǂ���

private:
	int LoadGeoid2000();
	int LoadEGM96();

	int ExtractByBilinear
	(
		const double& inB,
		const double& inL,
		double* outHg
	);

	int ExtractForEGM96
	(
		const double& inB,
		const double& inL,
		double* outHg
	);

	int CalcValueByWeightedAveMethod
	(
		CGeoidData* inZ11,
		CGeoidData* inZ12,
		CGeoidData* inZ21,
		CGeoidData* inZ22,
		double& inPx,
		double& inPy,
		double* outHg
	);

public:
	static const int GEOID_GEOID2000 = 1;
	static const int GEOID_EGM96 = 2;

	static const int NOERROR_ = 0;
	static const int TRANS_ERROR = 1;
	static const int FILE_NONEXIST = 10;
	static const int FILE_UNOPEN = 11;
	static const int FILE_ERROR = 12;

	CGeoid(int inGeoidType, wchar_t* inGeoidPath, double inOrgB, double inOrgL);
	virtual ~CGeoid();

	int Load();

	int Extract
	(
		const double& inB,
		const double& inL,
		double* outHg
	);

	static void nSpline(double* x, double* y, int N, double* a, double* b, double* c);//�X�v���C���֐��W���Z�o
	static double calSpline(double xp, double* x, double* y, double* a, double* b, double* c, int N);//�X�v���C����ԏ���
	static void f_tri(double* a, double* b, double* c, double* x, double* y, int N);//�X�v���C���֐��W���Z�o�̂��߂̈ꎟ��������@����

	static void nSpline4(double* x, double* y, double* a, double* b, double* c);//�X�v���C���֐��W���Z�o
	static double calSpline4(double xp, double* x, double* y, double* a, double* b, double* c);//�X�v���C����ԏ���
	static void f_tri2(double* a, double* b, double* c, double* x, double* y);//�X�v���C���֐��W���Z�o�̂��߂̈ꎟ��������@����
};
