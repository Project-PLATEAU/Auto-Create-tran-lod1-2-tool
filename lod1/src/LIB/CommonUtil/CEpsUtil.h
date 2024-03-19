#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

/*!
@brief	��r�֐����C�u����
@note	���̃N���X�ł�float,double���͂��߁A
<br>	�e��ϐ��̍ő�l��ŏ��l���`���A�����̔�r�֐���񋟂���B
<br>	�e��v���W�F�N�g�ł͂����Œ񋟂����ϐ��A
<br>	�֐����g���Ĕ�r���Z���s�����ƁB
<br>	���ׂ�inline�Œ�`���邽�ߑ΋K�͌v�Z���K�v�ȏ����͂����ł͏����Ȃ�
*/
class  CEpsUtil
{
public:
	static bool Zero(const double& dValue);
	static bool Equal(const double& dSrc, const double& dDest);
	static bool Less(const double& dSrc, const double& dDest);
	static bool LessEqual(const double& dSrc, const double& dDest);
	static bool Greater(const double& dSrc, const double& dDest);
	static bool GreaterEqual(const double& dSrc, const double& dDest);

	// FLOAT
	const static float _FLTMAX;		//!< float�̍ő�l
	const static float _FLTMIN;		//!< float�̍ŏ��l
	const static float _FLTEPS;		//!< float�ŕ\���\�Ȑ�Βl�̍ŏ��l

	static bool Zero(const float& dValue);
	static bool Equal(const float& fSrc, const float& fDest);
	static bool Less(const float& fSrc, const float& fDest);
	static bool LessEqual(const float& fSrc, const float& fDest);
	static bool Greater(const float& fSrc, const float& fDest);
	static bool GreaterEqual(const float& fSrc, const float& fDest);

	// �����擾
	//   �ŏ��l�A�ő�l��^�����͈͓̔��̗�����߂�
	static int    Rand(int minValue, int maxValue);
	static double Rand(const double& minValue, const double& maxValue);

	// Log�֐�
	static double LogBase(const double& a, const double& base);

	// �ő�E�ŏ�
	static double Min(const double& a, const double& b);
	static double Max(const double& a, const double& b);

};

/*! 0�l���f
@retval	true	0
@retval	false	0�ȊO
@note	double�̒l��0���𔻒f���܂��B
*/
inline bool CEpsUtil::Zero(const double& dValue	//!< in	�]������l
)
{
	return(fabs(dValue) < DBL_EPSILON);
}

/*! <���f
@retval	true	dSrc <  dDest
@retval	false	dSrc >= dDest
@note	2��double�l���r���܂��B
*/
inline bool CEpsUtil::Less(const double& dSrc,	//!< in	��r��
	const double& dDest	//!< in	��r��
)
{
	return ((dSrc < dDest) && !Equal(dSrc, dDest));
}

/*! <=���f
@retval	true	dSrc <= dDest
@retval	false	dSrc >  dDest
@note	2��double�l���r���܂��B
*/
inline bool CEpsUtil::LessEqual(const double& dSrc,	//!< in	��r��
	const double& dDest	//!< in	��r��
)
{
	return ((dSrc < dDest) || Equal(dSrc, dDest));
}

/*! >���f
@retval	true	dSrc >  dDest
@retval	false	dSrc <= dDest
@note	2��double�l���r���܂��B
*/
inline bool CEpsUtil::Greater(const double& dSrc,	//!< in	��r��
	const double& dDest	//!< in	��r��
)
{
	return (!LessEqual(dSrc, dDest));
}

/*! >=���f
@retval	true	dSrc >= dDest
@retval	false	dSrc <  dDest
@note	2��double�l���r���܂��B
*/
inline bool CEpsUtil::GreaterEqual(const double& dSrc,	//!< in	��r��
	const double& dDest	//!< in	��r��
)
{
	return (!Less(dSrc, dDest));
}


/*! =���f
@retval	true	dSrc == dDest
@retval	false	dSrc != dDest
@note	2��double�l���r���܂��B
*/
inline bool CEpsUtil::Equal(const double& dSrc,	//!< in	��r��
	const double& dDest)	//!< in	��r��
{
	if (Zero(dSrc)) {
		if (Zero(dDest)) return(true);
		else  	     return(false);
	}
	else {
		double d = DBL_EPSILON * dSrc;
		if (_isnan(d)) return false;
		return (fabs(dSrc - dDest) < fabs(DBL_EPSILON));
	}
}

/*! 0�l���f
@retval	true	0
@retval	false	0�ȊO
@note	float�̒l��0���𔻒f���܂��B
*/
inline bool CEpsUtil::Zero(const float& fValue	//!< in	�]������l
)
{
	return(fabs((double)fValue) < _FLTEPS);
}

/*! <���f
@retval	true	fSrc <  fDest
@retval	false	fSrc >= fDest
@note	2��float�l���r���܂��B
*/
inline bool CEpsUtil::Less(const float& fSrc,		//!< in	��r��
	const float& fDest		//!< in	��r��
)
{
	return ((fSrc < fDest) && !Equal(fSrc, fDest));
}

/*! <=���f
@retval	true	fSrc <= fDest
@retval	false	fSrc >  fDest
@note	2��float�l���r���܂��B
*/
inline bool CEpsUtil::LessEqual(const float& fSrc,		//!< in	��r��
	const float& fDest		//!< in	��r��
)
{
	return ((fSrc < fDest) || Equal(fSrc, fDest));
}

/*! >���f
@retval	true	fSrc >  fDest
@retval	false	fSrc <= fDest
@note	2��float�l���r���܂��B
*/
inline bool CEpsUtil::Greater(const float& fSrc,		//!< in	��r��
	const float& fDest		//!< in	��r��
)
{
	return (!LessEqual(fSrc, fDest));
}

/*! >=���f
@retval	true	fSrc >= fDest
@retval	false	fSrc <  fDest
@note	2��float�l���r���܂��B
*/
inline bool CEpsUtil::GreaterEqual(const float& fSrc,	//!< in	��r��
	const float& fDest	//!< in	��r��
)
{
	return (!Less(fSrc, fDest));
}

/*! =���f
@retval	true	fSrc == fDest
@retval	false	fSrc != fDest
@note	2��float�l���r���܂��B
*/
inline bool CEpsUtil::Equal(const float& fSrc,		//!< in	��r��
	const float& fDest		//!< in	��r��
)
{
	if (Zero(fSrc)) {
		if (Zero(fDest)) return(true);
		else  	     return(false);
	}
	else {
		double d = _FLTEPS * (double)fSrc;
		if (_isnan(d)) return false;
		return (fabs((double)fSrc - (double)fDest) < fabs(d));
	}
}

/*! �����擾
@return	�擾��������
@note	�ŏ��l�A�ő�l��^�����͈͓̔��̗�����߂�
*/
inline int CEpsUtil::Rand(int minValue,	//!< in	�ŏ��l
	int maxValue	//!< in	�ő�l
)
{
	int    number = rand();

	if (number == 0)        return minValue;
	if (number == RAND_MAX) return maxValue;

	double d = (double)number / (double)RAND_MAX; // 0.0�`1.0�̕�����
	return (int)(Min(minValue, maxValue) + d * abs(maxValue - minValue));
}

/*! �����擾
@return	�擾��������
@note	�ŏ��l�A�ő�l��^�����͈͓̔��̗�����߂�
*/
inline double CEpsUtil::Rand(const double& minValue,	//!< in	�ŏ��l
	const double& maxValue		//!< in	�ő�l
)
{
	int    number = rand();

	if (number == 0)
		return minValue;
	if (number == RAND_MAX)
		return maxValue;

	double d = (double)number / (double)RAND_MAX; // 0.0�`1.0�̕�����
	return (double)(Min(minValue, maxValue) + d * fabs(maxValue - minValue));
}

/*! ���w�ILOG�v�Z
@return �v�Z�l
*/
inline double CEpsUtil::LogBase(const double& a, const double& base)
{
	if (Zero(a) || Zero(base)) return - DBL_MAX;

	return log(a) / log(base);

}

/*! �ŏ�
@return �ŏ���Ԃ�
*/
inline double CEpsUtil::Min(const double& a, const double& b)
{
	return Less(a, b) ? a : b;
}

/*! �ő�
@return �ő��Ԃ�
*/
inline double CEpsUtil::Max(const double& a, const double& b)
{
	return Greater(a, b) ? a : b;
}
