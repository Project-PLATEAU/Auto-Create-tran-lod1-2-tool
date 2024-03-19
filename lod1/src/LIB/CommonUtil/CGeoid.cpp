#include "pch.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cwchar>
#include <string>

#include "CGeoid.h"
#include "CFileUtil.h"


//--------------------------------------------------------------------
// �r�b�O�G���f�B�A���̃��g���G���f�B�A���̕ϊ�
#define BS_BYTE1(x) (  x        & 0xFF )
#define BS_BYTE2(x) ( (x >>  8) & 0xFF )
#define BS_BYTE3(x) ( (x >> 16) & 0xFF )
#define BS_BYTE4(x) ( (x >> 24) & 0xFF )

#define BS_INT16(x) ( (unsigned short)(BS_BYTE1(x)<<8 | BS_BYTE2(x)) )
#define BS_INT32(x) ( BS_BYTE1(x)<<24 | BS_BYTE2(x)<<16 | BS_BYTE3(x)<<8 | BS_BYTE4(x) )

const int GEOID_NODATA = 1;

//-----------------------------------------------------------------------------
CGeoid::CGeoid
(
	int inGeoidType,
	wchar_t* inGeoidPath,
	double inOrgB,
	double inOrgL
)
{
	GeoidType = inGeoidType;
	wcscpy_s(GeoidPath, wcslen(inGeoidPath) * 2, inGeoidPath);

	OriginB = inOrgB;
	OriginL = inOrgL;

	Data = NULL;
}
//-----------------------------------------------------------------------------
CGeoid::~CGeoid()
{
	if (Data != NULL)
	{
		for (int i = 0; i < Height; i++) delete Data[i];
		delete[]Data;
	}
}
//-----------------------------------------------------------------------------
int CGeoid::Load()
{
	// �W�I�C�h�t�@�C�����݃`�F�b�N
	if (!GetFUtil()->IsExistPath(std::wstring(GeoidPath))) return FILE_NONEXIST;

	int ret;

	switch (GeoidType)
	{
		// ���{�̃W�I�C�h2000
	case GEOID_GEOID2000:
		ret = LoadGeoid2000();
		break;

		// ���E�̃W�I�C�h�iEGM96�j
	case GEOID_EGM96:
		ret = LoadEGM96();
		break;

	default:
		ret = FILE_ERROR;
		break;
	}

	return ret;
}

/*-----------------------------------------------------------------------------
�����{��GEOID2000

�����ȉ~�́FGRS80 / ITRF94

�W�I�C�h�E���f���́A�k��20�x����50�x�A���o120�x����150�x�͈̔͂ɂ��āA
�ܓx�Ԋu1���A�o�x�Ԋu1.5���̊i�q�ʒu�ɂ�����W�I�C�h���Ƃ��ė^�����Ă��܂��B

�e�L�X�g�iASCII�j�`���̃W�I�C�h�����b�V���E�t�@�C���egsigeome.ver4�f�́A
�P�s�ڂ��w�b�_�s�A�Q�s�ڈȍ~�Ɋi�q�_�ł̃W�I�C�h�������̏��ԂɊi�[����Ă��܂��B

��[�̈ܐ��i�k��20�x�j����k�Ɍ������k�[�̈ܐ��i�k��50�x�j�̏��ŁA
�e�ܐ��ɂ��Đ��[�i���o120�x�j���瓌�[�i���o150�x�j�̏��ŃW�I�C�h���̐��l�����P�ʂŊi�[����Ă��܂��B

�f�[�^�̏����́A���̂Ƃ���ł��B

	�i�w�b�_�s�F2F10.5,2F9.6,2I5,I2,A8�j
	glamn,glomn,dgla,dglo,nla,nlo,ikind,vern

	glamn�@�� 20.0�@�F��[�̈ܓx�l�i�k��20�x�j
	glomn�@��120.0�@�F���[�̌o�x�l�i���o120�x�j
	dgla�@ ��.016667�F�ܓx�Ԋu�i�x�P�ʁA�P���Ԋu�F�A���A�ۂ߂Ă���܂��j
	dglo�@ ��.025000�F�o�x�Ԋu�i�x�P�ʁA1.5���Ԋu�j
	nla�@�@�� 1801�@�F�ܐ��̌�
	nlo�@�@�� 1201�@�F�o���̌�
	ikind�@�� 1�@�@ �F�t�H�[�}�b�g���ʎq�i�����ł͓��ɈӖ��͂���܂���j
	vern�@ �� ver4.0�F�W�I�C�h�����b�V���E�t�@�C���̃o�[�W����

	�i�f�[�^�s�F28F9.4�j
	���ܐ����Ƃ�1201�̃f�[�^���J��Ԃ�����(28F9.4)�ŕ���ł��܂��B
-----------------------------------------------------------------------------*/
int CGeoid::LoadGeoid2000()
{
	int i, j;
	FILE* fp;
	wchar_t buf[1024];

	int format;
	wchar_t ver[32];

	// �W�I�C�h�t�@�C���I�[�v��	
	if (_wfopen_s(&fp, GeoidPath, L"r"))
	{
		return FILE_UNOPEN;
	}

	int fullH, fullW;
	double orgMinB, orgMinL;
	double orgDB, orgDL;

	// �w�b�_�[�s
	fgetws(buf, 1000, fp);

	int numTokens = swscanf_s(buf, L"%lf %lf %lf %lf %d %d %d %s",
		&orgMinB, &orgMinL, &orgDB, &orgDL, &fullH, &fullW, &format, ver, 32);

	if (numTokens != 8)
	{
		fclose(fp);
		return FILE_ERROR;
	}

	// ��O�l�Z�b�g
	Exception = 999.0;

	DB = 1.0 / 60;   // �ܓx�����F1.0����
	DL = 1.5 / 60;   // �o�x�����F1.5����

	// �ǂݍ��ݔ͈͌���
	// �����ł́A�ܓx�����S���i30���j�A�o�x�����F�}3����ǂݍ���
	// = �c1801 �~ ��241 �~ 16[byte] �� 7[MB]

	// �ܓx�����i�Œ�j
	MinB = 20.0;
	MaxB = 50.0;
	Height = 1801;

	// �o�x����
	double orgMaxL = 150.0;
	Width = 241;

	double tempMinL = OriginL - 3.0;
	double tempMaxL = OriginL + 3.0;

	int startCol;

	if (tempMaxL < orgMinL || orgMaxL < tempMinL)
	{ // �K�v�ȃf�[�^���͈͊O�̂Ƃ�

		fclose(fp);
		return FILE_ERROR;
	}

	if (tempMinL < orgMinL)
	{ // �K�v�ȃf�[�^�̐��[���f�[�^�͈͂̐��[��萼�̏ꍇ

		startCol = 0;
		MinL = orgMinL;
		MaxL = MinL + 6.0;
	}
	else if (orgMaxL < tempMaxL)
	{ // �K�v�ȃf�[�^�̓��[���f�[�^�͈͂̓��[��蓌�̏ꍇ

		startCol = 960;
		MaxL = orgMaxL;
		MinL = orgMaxL - 6.0;
	}
	else
	{
		startCol = (int)((tempMinL - orgMinL) / DL);
		MinL = orgMinL + DL * startCol;
		MaxL = orgMinL + DL * (startCol + Width - 1);
	}

	Data = new CGeoidData * [Height];
	for (i = 0; i < Height; i++) Data[i] = new CGeoidData[Width];

	CGeoidData* p;
	double dammy;

	int restCol = 1201 - startCol - Width;

	for (i = 0; i < Height; i++)
	{
		p = Data[i];

		for (j = 0; j < startCol; j++) fwscanf_s(fp, L"%lf", &dammy);

		for (j = 0; j < Width; j++)
		{
			fwscanf_s(fp, L"%lf", &p->Value);

			if (fabs(p->Value - Exception) < 0.00001)
			{
				p->Flag = GEOID_NODATA;
			}
			else
			{
				p->Flag = 0;
			}

			p++;
		}

		for (j = 0; j < restCol; j++) fwscanf_s(fp, L"%lf", &dammy);
	}

	fclose(fp);

	return NOERROR_;
}

/*-----------------------------------------------------------------------------
�����E�̃W�I�C�h�iEGM96�j

---- FILE: WW15MGH.DAC

The total size of the file is 2,076,480 bytes. This file was created
using an INTEGER*2 data type format and is an unformatted direct access
file. The data on the file is arranged in records from north to south.
There are 721 records on the file starting with record 1 at 90 N. The
last record on the file is at latitude 90 S. For each record, there
are 1,440 15 arc-minute geoid heights arranged by longitude from west to
east starting at the Prime Meridian (0 E) and ending 15 arc-minutes west
of the Prime Meridian (359.75 E). On file, the geoid heights are in units
of centimeters. While retrieving the Integer*2 values on file, divide by
100 and this will produce a geoid height in meters.
-----------------------------------------------------------------------------*/
int CGeoid::LoadEGM96()
{
	int i, j, k;
	FILE* fp;

	// �W�I�C�h�t�@�C���I�[�v��
	if (_wfopen_s(&fp, GeoidPath, L"rb"))
	{
		return FILE_UNOPEN;
	}

	DB = 15.0 / 60.0;   // �ܓx�����F15����
	DL = 15.0 / 60.0;   // �o�x�����F15����

	// �ܓx�����͑S�ēǂݍ���
	Height = 721;
	MinB = -90.0;
	MaxB = 90.0;

	// �o�x�����́A���W�n���_�}30��(=241���b�V���j
	Width = 241;

	// EGM��0��-360���͈̔͂Ȃ̂ŁA
	// ���o�̏ꍇ�́A�ꎞ�I�ɍ��W�n���_��360���𑫂�
	double tempOriginL = OriginL;

	double orgMinL = 0.0;
	double orgMaxL = 359.75;
	double tempMinL = tempOriginL - 30.0;
	double tempMaxL = tempOriginL + 30.0;

	int startCol;

	if (tempMinL < orgMinL)
	{ // �K�v�ȃf�[�^�̐��[��0����萼�i���o�̏ꍇ�j

		tempMinL += 360.0;
		startCol = (int)((tempMinL - orgMinL) / DL);

		if (tempMinL < 180.0)
		{ // ���t�ύX�����܂����ł���Ƃ��A
		  // 0 < MinL < 180, 180 < MaxL < 360 �Ƃ���

			MinL = orgMinL + DL * startCol;
			MaxL = MinL + 60.0;
			CrossDateLine = true;
		}
		else
		{ // ���t�ύX�����܂����ł��Ȃ��Ȃ�΁A
		  // -180 < MinL, MaxL �Ƃ���

			MinL = orgMinL + DL * startCol - 360.0;
			MaxL = MinL + 60.0;
			CrossDateLine = false;
		}
	}
	else
	{ // �K�v�ȃf�[�^�̐��[��0����蓌�i���o�j�̏ꍇ

		startCol = (int)((tempMinL - orgMinL) / DL);
		MinL = orgMinL + DL * startCol;
		MaxL = MinL + 60.0;

		if (180.0 < tempMaxL)
		{ // �K�v�ȃf�[�^�̓��[�����o180����蓌�̏ꍇ�A���t�ύX�����܂���

			CrossDateLine = true;
		}
		else
		{
			CrossDateLine = false;
		}
	}

	Data = new CGeoidData * [Height];
	for (i = 0; i < Height; i++) Data[i] = new CGeoidData[Width];

	int orgWidth = 1440;
	short* dat = new short[orgWidth];
	short val;

	for (i = Height - 1; i >= 0; i--) // �k����
	{
		fread(dat, 2, orgWidth, fp);

		for (j = 0, k = startCol; j < Width; j++, k++)
		{
			// ���[�i=360���j�܂ł���΁A���[�i=0���j�ɂ��ǂ�
			if (k == orgWidth) k = 0;

			val = BS_INT16(dat[k]);
			Data[i][j].Value = (double)val / 100.0;
		}
	}

	delete[]dat;

	fclose(fp);

	return NOERROR_;
}
//-----------------------------------------------------------------------------
// �W�I�C�h���o���s��
int CGeoid::Extract
(
	const double& inB,
	const double& inL,
	double* outHg
)
{
	int ret;

	switch (GeoidType)
	{
	case GEOID_GEOID2000:
		return ExtractByBilinear(inB, inL, outHg);

	case GEOID_EGM96:
		return ExtractForEGM96(inB, inL, outHg);

	default:
		ret = FILE_ERROR;
		break;
	}

	return ret;
}
//-----------------------------------------------------------------------------
// �o�C���j�A�ɂ���ԂŃW�I�C�h�����擾����
int CGeoid::ExtractByBilinear
(
	const double& inB,
	const double& inL,
	double* outHg
)
{
	// �쐼�̃��b�V��Index
	int idxRow;
	int idxCol;
	CGeoidData* z11, * z12, * z21, * z22;

	// �쐼�̃��b�V���C���f�b�N�X�擾
	idxCol = (int)(floor((inL - MinL) / DL));

	if (idxCol < 0 || Width <= idxCol)
	{
		return TRANS_ERROR;
	}

	idxRow = (int)(floor((inB - MinB) / DB));

	if (idxRow < 0 || Height <= idxRow)
	{
		return TRANS_ERROR;
	}

	z11 = &Data[idxRow][idxCol];
	z12 = &Data[idxRow][idxCol + 1];
	z21 = &Data[idxRow + 1][idxCol];
	z22 = &Data[idxRow + 1][idxCol + 1];

	//
	//    z21-------z22
	//     |     |   |
	//  1-s|     |   |
	//     |-----p---|
	//    s|     |   |
	//     |     |   |
	//    z11-------z12
	//        t   1-t

	double t = (inL - (MinL + DL * idxCol)) / DL;
	double s = (inB - (MinB + DB * idxRow)) / DB;

	if (!(z11->Flag & GEOID_NODATA) &&
		!(z12->Flag & GEOID_NODATA) &&
		!(z21->Flag & GEOID_NODATA) &&
		!(z22->Flag & GEOID_NODATA))
	{ // ��O�l���Ȃ��ꍇ

		*outHg = s * (t * z22->Value + (1.0 - t) * z21->Value) + (1.0 - s) * (t * z12->Value + (1.0 - t) * z11->Value);
	}
	else
	{ // �P�ł���O�l���܂ޏꍇ

		// ����l�̑��݂���ӏ��̂ݎg�p���āA�d�ݕt�����ϖ@�ŎZ�o����
		// �ЂƂ��f�[�^���Ȃ���΁A0.0��Ԃ�
		return CalcValueByWeightedAveMethod(z11, z12, z21, z22, t, s, outHg);
	}

	return NOERROR_;
}
//-----------------------------------------------------------------------------
// EGM96�̃W�I�C�h���o���s���i�X�v���C����ԁj
int CGeoid::ExtractForEGM96
(
	const double& inB,
	const double& inL,
	double* outHg
)
{
	// EGM�͌o�x�͈͂�0���`360���Ƃ���
	// 0���F�O���j�b�W�q�ߐ�
	double tempL = inL;

	if (CrossDateLine)
	{ // ���t�ύX�����܂����ł���Ƃ�

		if (inL < 0.0)
		{ // ���o�̂Ƃ��A+360������

			tempL += 360.0;
		}
	}

	int	i, j;
	double s, t;
	double gh[4], gf[4], gx[4], a[4], b[4], c[4];

	int idxRow;
	int idxCol;

	// ���b�V���C���f�b�N�X�擾
	idxCol = (int)(floor((tempL - MinL) / DL));

	if (idxCol < 0 || Width <= idxCol)
	{
		return TRANS_ERROR;
	}

	idxRow = (int)(floor((inB - MinB) / DB));

	if (idxRow < 0 || Height <= idxRow)
	{
		return TRANS_ERROR;
	}

	/*--------------------------

	+: ���߂�_

		|- t -|

		----------------
		|    |    |    |
		|    |    |    |
		----------------
		|    |    |    |
		|    |+   |    |  ---
 idxRow	----------------   |
		|    |    |    |   s
		|    |    |    |   |
		----------------  ---
		  idxCol

	--------------------------*/

	if (idxRow == 0 || idxRow == Height - 2 || idxCol == 0 || idxCol == Width - 2)
	{ // �[�����̂Ƃ��́A�o�C���j�A��ԂŎ擾

		return ExtractByBilinear(inB, inL, outHg);
	}

	s = (inB - MinB - (idxRow - 1) * DB) / DB;
	t = (tempL - MinL - (idxCol - 1) * DL) / DL;

	gx[0] = 0.0;
	gx[1] = 1.0;
	gx[2] = 2.0;
	gx[3] = 3.0;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{ // �o�x�����̂S�_�̍������i�[

			gh[j] = Data[idxRow + (i - i)][idxCol + (j - 1)].Value;
		}

		// �o�x�����ɃX�v���C����ԌW���̌v�Z
		nSpline4(gx, gh, a, b, c);
		gf[i] = calSpline4(t, gx, gh, a, b, c);
	}

	// �ܓx�����ɃX�v���C����ԌW���̌v�Z
	nSpline4(gx, gf, a, b, c);
	*outHg = calSpline4(s, gx, gf, a, b, c);

	return NOERROR_;
}
//-----------------------------------------------------------------------------
int CGeoid::CalcValueByWeightedAveMethod
(
	CGeoidData* inZ11,
	CGeoidData* inZ12,
	CGeoidData* inZ21,
	CGeoidData* inZ22,
	double& inPx,
	double& inPy,
	double* outHg
)
{
	CGeoidData* g[2][2];
	g[0][0] = inZ11;
	g[0][1] = inZ12;
	g[1][0] = inZ21;
	g[1][1] = inZ22;

	// 4������_p�ւ̋����̓����v�Z����
	double r[2][2];
	r[0][0] = inPx * inPx + inPy * inPy;
	r[0][1] = (1 - inPx) * (1 - inPx) + inPy * inPy;
	r[1][0] = inPx * inPx + (1 - inPy) * (1 - inPy);
	r[1][1] = (1 - inPx) * (1 - inPx) + (1 - inPy) * (1 - inPy);

	double sumWZ = 0.0;
	double sumW = 0.0;
	double w;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			if (g[i][j]->Flag & GEOID_NODATA) continue;

			w = 1.0 / r[i][j];  // �d�݂͋����̓�敪�̂P
			sumWZ += w * g[i][j]->Value;
			sumW += w;
		}
	}

	if (sumW == 0.0)
	{
		*outHg = 0.0;
		return TRANS_ERROR;
	}

	*outHg = sumWZ / sumW;

	return NOERROR_;
}

//-----------------------------------------------------------------------------
void CGeoid::nSpline(double* x, double* y, int N, double* a, double* b, double* c)
{
	int i;
	double* h, * v, * u, * tmp;

	h = (double*)malloc(N * sizeof(double));
	v = (double*)malloc(N * sizeof(double));
	u = (double*)malloc(N * sizeof(double));
	tmp = (double*)malloc((N - 2) * sizeof(double));

	for (i = 0; i < N - 1; i++)
	{
		h[i] = x[i + 1] - x[i];
	}

	for (i = 1; i < N - 1; i++)
	{
		a[i - 1] = 2.0 * (h[i - 1] + h[i]);
		v[i - 1] = 6.0 * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);
	}

	for (i = 1; i < N - 1; i++) b[i - 1] = c[i - 1] = h[i];

	f_tri(a, b, c, tmp, v, N - 2);

	for (i = 1; i < N - 1; i++) u[i] = tmp[i - 1];

	u[0] = u[N - 1] = 0.0;

	for (i = 0; i < N - 1; i++)
	{
		a[i] = (u[i + 1] - u[i]) / (6.0 * h[i]);
		b[i] = 0.5 * u[i];
		c[i] = (y[i + 1] - y[i]) / h[i] - h[i] * (2.0 * u[i] + u[i + 1]) / 6.0;
	}

	free(h);
	free(v);
	free(u);
	free(tmp);

	return;
}

//-----------------------------------------------------------------------------
void CGeoid::nSpline4
(
	double* x, // [i/ ] x[0]=0, x[1]=1, x[2]=2, x[3]=3 
	double* y, // [i/ ] x[i]�ł̒ly[i] �����ł͕W��
	double* a, // [ /o] ���߂�W��a[4]
	double* b, // [ /o] ���߂�W��b[4]
	double* c  // [ /o] ���߂�W��c[4]
)
{
	int i;
	double h[4], v[4], u[4], tmp[2];

	for (i = 0; i < 3; i++)
	{
		h[i] = x[i + 1] - x[i];  // h[0]-h[2]
	}

	for (i = 1; i < 3; i++)
	{
		a[i - 1] = 2.0 * (h[i - 1] + h[i]);                                      // a[0]-a[1]
		v[i - 1] = 6.0 * ((y[i + 1] - y[i]) / h[i] - (y[i] - y[i - 1]) / h[i - 1]);  // v[0]-v[1]
	}

	for (i = 1; i < 3; i++) b[i - 1] = c[i - 1] = h[i]; // b[0]-b[1], c[0]-c[1]

	f_tri2(a, b, c, tmp, v);

	for (i = 1; i < 3; i++) u[i] = tmp[i - 1];  // u[1]-u[2]

	u[0] = u[3] = 0.0;

	for (i = 0; i < 3; i++)
	{
		a[i] = (u[i + 1] - u[i]) / (6.0 * (x[i + 1] - x[i]));
		b[i] = 0.5 * u[i];
		c[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]) - (x[i + 1] - x[i]) * (2.0 * u[i] + u[i + 1]) / 6.0;
	}

	return;
}

//-----------------------------------------------------------------------------
/* �X�v���C����Ԓl�̌v�Z�@*/
double CGeoid::calSpline(double x1, double* x, double* y, double* a, double* b, double* c, int N)
{
	int i;
	double xx, y1;

	// ��Ԃ̌���
	for (i = 0; i < N; i++) {
		if (x1 <= x[i + 1]) break;
	}

	if (i == N - 1) i--;

	xx = x1 - x[i];
	y1 = y[i] + (c[i] + (b[i] + a[i] * xx) * xx) * xx;

	return y1;
}
//-----------------------------------------------------------------------------
/* �X�v���C����Ԓl�̌v�Z�@*/
double CGeoid::calSpline4
(
	double x1,  // ��_����̒P�ʋ���[mesh] 
	double* x,  // �P�ʋ��� x[0]=0, x[1]=1, x[2]=2, x[3]=3
	double* y,  // x[i]�ł̒ly[i]�i�����ł́A�W���j
	double* a,  // �W��a
	double* b,  // �W��b
	double* c   // �W��c
)
{
	int i;
	double xx, y1;

	// ��Ԃ̌���
	for (i = 0; i < 3; i++)
	{
		if (x1 <= x[i + 1]) break;
	}

	if (i == 3) i--;

	xx = x1 - x[i];
	y1 = y[i] + (c[i] + (b[i] + a[i] * xx) * xx) * xx;

	return y1;
}

//-----------------------------------------------------------------------------
/* �A���P���������������@*/
void CGeoid::f_tri(double* a, double* b, double* c, double* x, double* y, int N)
{
	int i;
	double* d, * z;

	d = (double*)malloc(N * sizeof(double));
	z = (double*)malloc(N * sizeof(double));

	/* LU�����@*/
	d[0] = a[0];
	for (i = 1; i < N; i++) {
		x[i] = b[i] / d[i - 1];
		d[i] = a[i] - x[i] * c[i - 1];
	}

	/* �O�i����@*/
	z[0] = y[0];
	for (i = 1; i < N; i++)	z[i] = y[i] - x[i] * z[i - 1];

	/*  ��ޑ���@*/
	x[N - 1] = z[N - 1] / d[N - 1];
	for (i = N - 2; i >= 0; i--) {
		x[i] = (z[i] - c[i] * x[i + 1]) / d[i];
	}

	free(d);
	free(z);

	return;
}
//-----------------------------------------------------------------------------
/* �A���P���������������@*/
void CGeoid::f_tri2(double* a, double* b, double* c, double* x, double* y)
{
	double d[2], z[2];

	/* LU�����@*/
	d[0] = a[0];
	x[1] = b[1] / d[0];
	d[1] = a[1] - x[1] * c[0];

	/* �O�i����@*/
	z[0] = y[0];
	z[1] = y[1] - x[1] * z[0];

	/*  ��ޑ���@*/
	x[1] = z[1] / d[1];
	x[0] = (z[0] - c[0] * x[1]) / d[0];

	return;
}
