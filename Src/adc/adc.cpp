#include "adc/adc.h"

static ADC_HandleTypeDef adc_handle;
static uint32_t adc_channel_count;

#define ADC_CONVERSION_TIMEOUT      10
#define ADC_MAX_SEQUENCE_CHANNELS   16

// Налаштування пінів GPIO для каналів АЦП
static void ConfigureChannelPin(uint32_t channel)
{
	GPIO_TypeDef *port = nullptr;
	uint16_t pin = 0;

	if (channel <= ADC_CHANNEL_7)
	{
		port = GPIOA;
		pin = GPIO_PIN_0 << channel;
	}
	else if (channel == ADC_CHANNEL_8 || channel == ADC_CHANNEL_9)
	{
		port = GPIOB;
		pin = channel == ADC_CHANNEL_8 ? GPIO_PIN_0 : GPIO_PIN_1;
	}
	else if (channel <= ADC_CHANNEL_15)
	{
		port = GPIOC;
		pin = GPIO_PIN_0 << (channel - ADC_CHANNEL_10);
	}

	if (port != nullptr)
	{
		GPIO_InitTypeDef gpio_config = {0};
		gpio_config.Pin = pin;
		gpio_config.Mode = GPIO_MODE_ANALOG;
		gpio_config.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(port, &gpio_config);
	}
}

// Ініціалізація АЦП з вказаними каналами та кількістю каналів
HAL_StatusTypeDef ADC_Init(const uint32_t *channels, uint32_t count)
{
	if (channels == nullptr || count == 0U || count > ADC_MAX_SEQUENCE_CHANNELS)
	{
		return HAL_ERROR;
	}

	adc_channel_count = 0U;

    // Ініціалізація тактування АЦП та портів GPIO для каналів
	__HAL_RCC_ADC1_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

    // Ініціалізація структури обробника АЦП
	adc_handle.Instance = ADC1;
	adc_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	adc_handle.Init.Resolution = ADC_RESOLUTION_12B;
	adc_handle.Init.ScanConvMode = count > 1 ? ENABLE : DISABLE;
	adc_handle.Init.ContinuousConvMode = DISABLE;
	adc_handle.Init.DiscontinuousConvMode = DISABLE;
	adc_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	adc_handle.Init.NbrOfConversion = count;
	adc_handle.Init.DMAContinuousRequests = DISABLE;
	adc_handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    // Налаштування каналів АЦП
	for (uint32_t index = 0; index < count; ++index)
	{
		if (channels[index] > ADC_CHANNEL_15)
		{
			return HAL_ERROR;
		}
		ConfigureChannelPin(channels[index]);
	}

    // Ініціалізація АЦП
	HAL_StatusTypeDef status = HAL_ADC_Init(&adc_handle);
	if (status != HAL_OK)
	{
		return status;
	}

    // Конфігурація каналів АЦП після ініціалізації
	for (int index = 0; index < count; ++index)
	{
		ADC_ChannelConfTypeDef channel_config = {0};
		channel_config.Channel = channels[index];
		channel_config.Rank = index + 1U;
		channel_config.SamplingTime = ADC_SAMPLETIME_84CYCLES;

		status = HAL_ADC_ConfigChannel(&adc_handle, &channel_config);
		if (status != HAL_OK)
		{
			return status;
		}
	}

    // Збереження кількості каналів АЦП після успішної ініціалізації
	adc_channel_count = count;

	return HAL_OK;
}

// Зчитування послідовності значень з АЦП
HAL_StatusTypeDef ADC_ReadSequence(uint32_t *values, uint32_t count)
{
	if (values == nullptr || count == 0U || count != adc_channel_count)
	{
		return HAL_ERROR;
	}

    // Запуск послідовного зчитування значень з АЦП
	HAL_StatusTypeDef status = HAL_ADC_Start(&adc_handle);
	if (status != HAL_OK)
	{
		return status;
	}

    // Зчитування значень з кожного каналу АЦП у послідовності
	for (int index = 0; index < count; ++index)
	{
		status = HAL_ADC_PollForConversion(&adc_handle,
                                            ADC_CONVERSION_TIMEOUT);
		if (status != HAL_OK)
		{
			break;
		}

		values[index] = HAL_ADC_GetValue(&adc_handle);
	}

    // Зупинка АЦП після завершення зчитування значень
	HAL_StatusTypeDef stop_status = HAL_ADC_Stop(&adc_handle);
	if (status == HAL_OK)
	{
		status = stop_status;
	}

	return status;
}
