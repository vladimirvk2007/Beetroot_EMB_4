#include "adc/adc.h"

static ADC_HandleTypeDef adc_handle;
static DMA_HandleTypeDef adc_dma_handle;
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

	for (uint32_t index = 0U; index < count; ++index)
	{
		if (channels[index] > ADC_CHANNEL_15)
		{
			return HAL_ERROR;
		}
	}

	adc_channel_count = 0U;

    // Ініціалізація тактування АЦП та портів GPIO для каналів
	__HAL_RCC_ADC1_CLK_ENABLE();
	__HAL_RCC_DMA2_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	adc_dma_handle.Instance = DMA2_Stream0;
	adc_dma_handle.Init.Channel = DMA_CHANNEL_0;
	adc_dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
	adc_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
	adc_dma_handle.Init.MemInc = DMA_MINC_ENABLE;
	adc_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
	adc_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
	adc_dma_handle.Init.Mode = DMA_NORMAL;
	adc_dma_handle.Init.Priority = DMA_PRIORITY_HIGH;
	adc_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	adc_dma_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
	adc_dma_handle.Init.MemBurst = DMA_MBURST_SINGLE;
	adc_dma_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;

	HAL_StatusTypeDef status = HAL_DMA_Init(&adc_dma_handle);
	if (status != HAL_OK)
	{
		return status;
	}

	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5U, 0U);
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    // Ініціалізація структури обробника АЦП
	adc_handle.Instance = ADC1;
	adc_handle.DMA_Handle = &adc_dma_handle;
	adc_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	adc_handle.Init.Resolution = ADC_RESOLUTION_12B;
	adc_handle.Init.ScanConvMode = count > 1 ? ENABLE : DISABLE;
	adc_handle.Init.ContinuousConvMode = DISABLE;
	adc_handle.Init.DiscontinuousConvMode = DISABLE;
	adc_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	adc_handle.Init.NbrOfConversion = count;
	adc_handle.Init.DMAContinuousRequests = ENABLE;
	adc_handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    // Налаштування каналів АЦП
	for (uint32_t index = 0; index < count; ++index)
	{
		ConfigureChannelPin(channels[index]);
	}

    // Ініціалізація АЦП
	status = HAL_ADC_Init(&adc_handle);
	if (status != HAL_OK)
	{
		return status;
	}

    // Конфігурація каналів АЦП після ініціалізації
	for (uint32_t index = 0U; index < count; ++index)
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

// Асинхронний запуск послідовності перетворень через DMA
HAL_StatusTypeDef ADC_StartSequenceDMA(uint32_t *values, uint32_t count)
{
	if (values == nullptr || count == 0U || count != adc_channel_count)
	{
		return HAL_ERROR;
	}

	return HAL_ADC_Start_DMA(&adc_handle, values, count);
}

// Зчитування послідовності значень з АЦП
HAL_StatusTypeDef ADC_ReadSequence(uint32_t *values, uint32_t count)
{
	if (values == nullptr || count == 0U || count != adc_channel_count)
	{
		return HAL_ERROR;
	}

	// У блокуючому режимі DMA завершується polling-ом замість IRQ callback.
	HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn);

    // Запуск перетворень і передавання результатів у буфер через DMA
	HAL_StatusTypeDef status = ADC_StartSequenceDMA(values, count);
	if (status != HAL_OK)
	{
		HAL_NVIC_ClearPendingIRQ(DMA2_Stream0_IRQn);
		HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
		return status;
	}

	uint32_t tick_start = HAL_GetTick();
	while (true)
	{
		if (__HAL_DMA_GET_FLAG(&adc_dma_handle,
		                       __HAL_DMA_GET_TE_FLAG_INDEX(&adc_dma_handle)) != RESET ||
		    __HAL_DMA_GET_FLAG(&adc_dma_handle,
		                       __HAL_DMA_GET_DME_FLAG_INDEX(&adc_dma_handle)) != RESET ||
		    __HAL_DMA_GET_FLAG(&adc_dma_handle,
		                       __HAL_DMA_GET_FE_FLAG_INDEX(&adc_dma_handle)) != RESET)
		{
			status = HAL_ERROR;
			break;
		}

		if (__HAL_DMA_GET_FLAG(&adc_dma_handle,
		                       __HAL_DMA_GET_TC_FLAG_INDEX(&adc_dma_handle)) != RESET)
		{
			status = HAL_DMA_PollForTransfer(&adc_dma_handle,
			                                 HAL_DMA_FULL_TRANSFER,
			                                 0U);
			break;
		}

		if ((HAL_GetTick() - tick_start) >= ADC_CONVERSION_TIMEOUT)
		{
			status = HAL_TIMEOUT;
			break;
		}
	}

    // Зупинка АЦП після завершення передавання DMA
	HAL_StatusTypeDef stop_status = HAL_ADC_Stop_DMA(&adc_handle);
	if (status == HAL_OK)
	{
		status = stop_status;
	}

	HAL_NVIC_ClearPendingIRQ(DMA2_Stream0_IRQn);
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

	return status;
}

// Передавання DMA2 Stream 0 до HAL для обробки завершення ADC
void ADC_DMA_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&adc_dma_handle);
}
