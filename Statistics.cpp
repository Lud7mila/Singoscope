/****************************************************************************************
*
*   ����������� ������ Statistics
*
*   ������ ������ �������� ���������� �������������� �������.
*
*   �����: ��������� ����������� � ������� ������������, 2009 - 2010
*
****************************************************************************************/

#include <windows.h>
#include <math.h>
#include <float.h>

#include "float_const.h"
#include "Statistics.h"

#include "Log.h"

	
/****************************************************************************************
*
*   ���������
*
****************************************************************************************/

// ����������� ���������� ���������� �� �������� (� ���������)
#define	 MAX_DEVIATION_FROM_MEAN	0.5



/****************************************************************************************
*
*   ��������� �������, ����������� ����
*
****************************************************************************************/

static double CalculateMean(double *pSample, DWORD nSamples,
							double drMin, double drMax);

static double CalculateStandDeviation(double *pSample, DWORD nSamples,
										double drMean);



/****************************************************************************************
*
*   ������� EstimateMeanOfPeriodicProcess
*
*   ���������
*       pSample  - ��������� �� �������
*		nSamples - ���������� ������ �������
*		drMin	 - ���������� ��������� ��������
*		drMax	 - ����������� ��������� ��������
*
*   ������������ ��������
*       ������� �������
*
*   ������ ������� ��������� ������� �������� �������������� ��������.
*   
*   ������ ������������ � 3 �����:
*      1. ����������� ������� ��� �������� �������
*      2. ������������ �������� ����� �������, ������� ����������� �� �������� �����,
*         ��� �� ��������� THRESHOLD_FOR_MEAN_ESTIMATION
*         ���������: ���� ���� ������� �� �������� � �������� ����� drMin � drMax
*                    ����������, �� ������ ���� ������� ����� ����� �������
*      3. ����������� ������� ��� ���������� (����� �����������) �������.
*
****************************************************************************************/

double EstimateMeanOfPeriodicProcess(double *pSample, DWORD cSamples,
									 double drMin, double drMax)
{
	// the first step
	double drFirstMean = CalculateMean(pSample, cSamples, drMin, drMax);
	
	if (_isnan(drFirstMean))
	{
		LOG("CalculateMean failed\n");		
		return INDETERMINATE;
	}
	

	// the second step
	double drSecondMean = 0;

	DWORD cRemainedSamples = 0;
	for (DWORD i = 0; i < cSamples; i++)
	{
		if (fabs(pSample[i] - drFirstMean) < MAX_DEVIATION_FROM_MEAN * drFirstMean)
		{
			drSecondMean += pSample[i];
			cRemainedSamples ++;
		}
	}

	if (cRemainedSamples == 0) return INDETERMINATE;

	drSecondMean /= cRemainedSamples;

	// the third step
	double drThirdMean = 0;
	DWORD RealCountOfSamples = 0;

	for (DWORD i = 0; i < cSamples; i++)
	{
		drThirdMean += pSample[i];
        
        RealCountOfSamples += (DWORD) (pSample[i] / drSecondMean); // !!!!!
	}

	if (RealCountOfSamples == 0) return INDETERMINATE;

	drThirdMean = drThirdMean / RealCountOfSamples;

	return drThirdMean;
}



/****************************************************************************************
*
*   ������� CalculateMean
*
*   ���������
*       pSample  - ��������� �� �������
*		nSamples - ���������� ������ �������
*		drMin	 - ���������� ��������� ��������
*		drMax	 - ����������� ��������� ��������
*
*   ������������ ��������
*       ������� �������,
*       INDETERMINATE - ��������� ������ �� ����� ���������� ��������
*
*   ������ ������� ��������� ������� ��� ��������� �������.
*
*   ���� ���� ������� �� �������� � �������� ����� drMin � drMax ����������, �� ��
*   �� ����� ����������� ��� ���������� ��������.
*
****************************************************************************************/

double CalculateMean(double *pSample, DWORD nSamples, double drMin, double drMax)
{
	double drMean = 0;
	DWORD cRemainedSamples = 0;

	for (DWORD i = 0; i < nSamples; i++)
		if (pSample[i] > drMin && pSample[i] < drMax)
		{
			drMean += pSample[i];
			cRemainedSamples ++;
		}
	
	if (cRemainedSamples == 0) return INDETERMINATE;

	drMean /= cRemainedSamples;

	return drMean;
}



/****************************************************************************************
*
*   ������� CalculateStandDeviation
*
*   ���������
*       pSample  - ��������� �� �������
*		nSamples - ���������� ������ �������
*
*   ������������ ��������
*       ��� ��� �������
*
*   ������ ������� ��������� ��� ��� ��������� �������.
*
****************************************************************************************/


static double CalculateStandDeviation(double *pSample, DWORD nSamples,
										double drMean)
{
	double drStandDeviation = 0;
	for (DWORD i = 0; i < nSamples; i++)
		drStandDeviation += (pSample[i] - drMean) *
							(pSample[i] - drMean);
	drStandDeviation /= nSamples;
	drStandDeviation = sqrt(drStandDeviation);

	return drStandDeviation;
}
