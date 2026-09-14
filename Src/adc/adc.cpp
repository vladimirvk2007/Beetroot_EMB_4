#include "adc/adc.h"

extern ADC_HandleTypeDef hadc1;

HAL_StatusTypeDef ADC_Read(uint32_t *value, uint32_t timeout)
{
	HAL_StatusTypeDef status;

	if (value == nullptr)
	{
		return HAL_ERROR;
	}

	status = HAL_ADC_Start(&hadc1);
	if (status != HAL_OK)
	{
		return status;
	}

	status = HAL_ADC_PollForConversion(&hadc1, timeout);
	if (status == HAL_OK)
	{
		*value = HAL_ADC_GetValue(&hadc1);
	}

	HAL_StatusTypeDef stop_status = HAL_ADC_Stop(&hadc1);
	if (status == HAL_OK)
	{
		status = stop_status;
	}

	return status;
}

HAL_StatusTypeDef ADC_ReadChannel(uint32_t channel, uint32_t *value, uint32_t timeout)
{
	ADC_ChannelConfTypeDef channel_config = {0};

	channel_config.Channel = channel;
	channel_config.Rank = 1;
	channel_config.SamplingTime = ADC_SAMPLETIME_3CYCLES;

	HAL_StatusTypeDef status = HAL_ADC_ConfigChannel(&hadc1, &channel_config);
	if (status != HAL_OK)
	{
		return status;
	}

	return ADC_Read(value, timeout);
}

HAL_StatusTypeDef ADC_ReadSequence(uint32_t *values, uint32_t count, uint32_t timeout)
{
	if (values == nullptr || count == 0U)
	{
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status = HAL_ADC_Start(&hadc1);
	if (status != HAL_OK)
	{
		return status;
	}

	for (uint32_t index = 0; index < count; ++index)
	{
		status = HAL_ADC_PollForConversion(&hadc1, timeout);
		if (status != HAL_OK)
		{
			break;
		}

		values[index] = HAL_ADC_GetValue(&hadc1);
	}

	HAL_StatusTypeDef stop_status = HAL_ADC_Stop(&hadc1);
	if (status == HAL_OK)
	{
		status = stop_status;
	}

	return status;
}
