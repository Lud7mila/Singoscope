/****************************************************************************************
*
*   Определение модуля Statistics
*
*   Данный модуль содержит реализации статистических функций.
*
*   Автор: Александр Огородников и Людмила Огородникова, 2009 - 2010
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
*   Константы
*
****************************************************************************************/

// максимально допустимое отклонение от среднего (в процентах)
#define	 MAX_DEVIATION_FROM_MEAN	0.5



/****************************************************************************************
*
*   Прототипы функций, определённых ниже
*
****************************************************************************************/

static double CalculateMean(double *pSample, DWORD nSamples,
							double drMin, double drMax);

static double CalculateStandDeviation(double *pSample, DWORD nSamples,
										double drMean);



/****************************************************************************************
*
*   Функция EstimateMeanOfPeriodicProcess
*
*   Параметры
*       pSample  - указатель на выборку
*		nSamples - количество членов выборки
*		drMin	 - минимально возможное значение
*		drMax	 - максимально возможное значение
*
*   Возвращаемое значение
*       среднее выборки
*
*   Данная функция оценивает среднее значение периодического процесса.
*   
*   Оценка производится в 3 этапа:
*      1. вычисляется среднее для исходной выборки
*      2. откидываются элементы члены выборки, которые отклоняются от среднего более,
*         чем на константу THRESHOLD_FOR_MEAN_ESTIMATION
*         Замечание: если член выборки не попадает в диапазон между drMin и drMax
*                    значениями, то данный член выборки будет также откинут
*      3. вычисляется среднее для полученной (после откидывания) выборки.
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
*   Функция CalculateMean
*
*   Параметры
*       pSample  - указатель на выборку
*		nSamples - количество членов выборки
*		drMin	 - минимально возможное значение
*		drMax	 - максимально возможное значение
*
*   Возвращаемое значение
*       среднее выборки,
*       INDETERMINATE - произошла ошибка во время вычисления среднего
*
*   Данная функция вычисляет среднее для указанной выборки.
*
*   Если член выборки не попадает в диапазон между drMin и drMax значениями, то он
*   не будет учитываться при вычислении среднего.
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
*   Функция CalculateStandDeviation
*
*   Параметры
*       pSample  - указатель на выборку
*		nSamples - количество членов выборки
*
*   Возвращаемое значение
*       СКО для выборки
*
*   Данная функция вычисляет СКО для указанной выборки.
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
