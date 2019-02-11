#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "utils.h"


float UTILS_Mean(float aData[], uint32_t numSamples)
{
    uint32_t idx;
    float sum = 0.0f;
    float mean;

    for (idx=0;idx<numSamples;idx++)
    {
        sum += aData[idx];
    }

    mean = sum / ((float)numSamples);

    return mean;
}

float UTILS_StandardDeviation(float aData[], uint32_t numSamples)
{
    float mean;
    float standardDeviation;
    uint32_t idx;
    float sum = 0.0f;

    mean = UTILS_Mean(aData, numSamples);
    for (idx=0;idx<numSamples;idx++)
    {
        float x = (aData[idx] - mean);
        sum += x * x;
    }
    standardDeviation = sqrtf(sum / (float)(numSamples - 1));

    return standardDeviation;
}

float UTILS_FilterData(float aData[], uint32_t numSamples, float factor)
{
    uint32_t idxSrc;
    uint32_t idxDst;
    float standardDeviation;
    float mean;
    uint32_t numSamples1;

    standardDeviation = UTILS_StandardDeviation(aData, numSamples);
    mean = UTILS_Mean(aData, numSamples);

    /*printf("mean: %f\n", mean);
    printf("std dev: %f\n", standardDeviation);*/

    numSamples1 = numSamples;
    for (idxSrc=0,idxDst=0;idxSrc<numSamples;idxSrc++,idxDst++)
    {
        if (aData[idxSrc] > mean - standardDeviation * factor)
        {
            aData[idxDst] = aData[idxSrc];
        }
        else
        {
            /* Ignore this value */
            //printf("remove: %f\n", aData[idxSrc]);
            idxDst--;
            numSamples1--;
        }
    }

    /*for (idxSrc=0;idxSrc<numSamples1;idxSrc++)
        printf("%f ", aData[idxSrc]);
    printf("\n");*/

    numSamples = numSamples1;
    for (idxSrc=0,idxDst=0;idxSrc<numSamples1;idxSrc++,idxDst++)
    {
        if (aData[idxSrc] < mean + standardDeviation * factor)
        {
            aData[idxDst] = aData[idxSrc];
        }
        else
        {
            /* Ignore this value */
            //printf("remove: %f\n", aData[idxSrc]);
            idxDst--;
            numSamples--;
        }
    }

    /*for (idxSrc=0;idxSrc<numSamples;idxSrc++)
        printf("%f ", aData[idxSrc]);
    printf("\n");*/

    return numSamples;
}

float UTILS_GetFilteredValueInArray(float aData[], uint32_t numSamples)
{
    float retVal = aData[0];

    numSamples = UTILS_FilterData(aData, numSamples, 2.0f);

    /* In case that all elements are equal numSamples will be 0 */
    if (numSamples)
    {
        retVal = UTILS_Mean(aData, numSamples);
    }

    return retVal;
}

float UTILS_ConvertCtoF(float c)
{
    return c * 1.8 + 32;
}

float UTILS_ConvertFtoC(float f) {
    return (f - 32) * 0.55555;
}

float UTILS_ComputeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit) {
    // Using both Rothfusz and Steadman's equations
    // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
    float hi;

    if (!isFahrenheit)
        temperature = UTILS_ConvertCtoF(temperature);

    hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

    if (hi > 79) {
        hi = -42.379 +
            2.04901523 * temperature +
            10.14333127 * percentHumidity +
            -0.22475541 * temperature*percentHumidity +
            -0.00683783 * pow(temperature, 2) +
            -0.05481717 * pow(percentHumidity, 2) +
            0.00122874 * pow(temperature, 2) * percentHumidity +
            0.00085282 * temperature*pow(percentHumidity, 2) +
            -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

        if((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
            hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

        else if((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
            hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
    }

    return isFahrenheit ? hi : UTILS_ConvertFtoC(hi);
}

