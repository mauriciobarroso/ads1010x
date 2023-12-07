/**
  ******************************************************************************
  * @file           : ads101x.c
  * @author         : Mauricio Barroso Benavides
  * @date           : Nov 6, 2023
  * @brief          : todo: write brief
  ******************************************************************************
  * @attention
  *
  * MIT License
  *
  * Copyright (c) 2023 Mauricio Barroso Benavides
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to
  * deal in the Software without restriction, including without limitation the
  * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  * sell copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  * 
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  * IN THE SOFTWARE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "ads101x.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

/* Private macros ------------------------------------------------------------*/
#define NOP() asm volatile ("nop")

/* External variables --------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const char *TAG = "ads101x";

/* Private function prototypes -----------------------------------------------*/
/**
 * @brief Function that implements the default I2C read transaction
 *
 * @param reg_addr : Register address to be read
 * @param reg_data : Pointer to the data to be read from reg_addr
 * @param data_len : Length of the data transfer
 * @param intf     : Pointer to the interface descriptor
 *
 * @return 0 if successful, non-zero otherwise
 */
static int8_t i2c_read(uint8_t reg_addr, uint16_t *reg_data, void *intf);

/**
 * @brief Function that implements the default I2C write transaction
 *
 * @param reg_addr : Register address to be written
 * @param reg_data : Data to be written to reg_addr
 * @param data_len : Length of the data transfer
 * @param intf     : Pointer to the interface descriptor
 *
 * @return 0 if successful, non-zero otherwise
 */
static int8_t i2c_write(uint8_t reg_addr, const uint16_t reg_data,
		                    void *intf);
/**
 * @brief Function that implements a micro seconds delay
 *
 * @param period_us: Time in us to delay
 */
static void delay_us(uint32_t period_us);

/* Exported functions definitions --------------------------------------------*/
/**
 * @brief Function to initialize a ADS101x instance
 */
esp_err_t ads101x_init(ads101x_t *const me, i2c_bus_t *i2c_bus, uint8_t dev_addr,
		                 i2c_bus_read_t read, i2c_bus_write_t write) {
	/* Print initializing message */
	ESP_LOGI(TAG, "Initializing instance...");

	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Add device to bus */
	ret = i2c_bus_add_dev(i2c_bus, dev_addr, "ads101x", NULL, NULL);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to add device");
		return ret;
	}

	/**/
	me->i2c_dev = &i2c_bus->devs.dev[i2c_bus->devs.num - 1]; /* todo: write function to get the dev from name */

	/* Print successful initialization message */
	ESP_LOGI(TAG, "Instance initialized successfully");

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads a specific single-ended ADC channel.
 */
esp_err_t ads101x_read_single_ended(ads101x_t *const me,
		                                ads101x_channel_t channel,
																		int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Check channel argument */
	if (me->model == ADS101X_MODEL_5) { /* ADS1015 */
		if (channel > 3) {
			ESP_LOGE(TAG, "Failed to select channel, must be less than 3");
			return ESP_FAIL;
		}
	}
	else { /* ADS1013 and ADS1014 */
		if (channel > 1) {
			ESP_LOGE(TAG, "Failed to select channel, must be less than 1");
			return ESP_FAIL;
		}
	}

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, channel, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	bool conversion_is_complete = false;

	do {
		delay_us(5 * 1000); /* Wait for 5 ms */
		ads101x_conversion_complete(me, &conversion_is_complete);
	} while (!conversion_is_complete);

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the voltage difference between the P (AIN0) and N
 *				(AIN1) input.
 */
esp_err_t ads101x_read_differential_0_1(ads101x_t *const me,
		                                    int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, ADS101X_REG_CONFIG_MUX_DIFF_0_1, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	bool conversion_is_complete = false;

	do {
		delay_us(5 * 1000); /* Wait for 5 ms */
		ads101x_conversion_complete(me, &conversion_is_complete);
	} while (!conversion_is_complete);

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the voltage difference between the P (AIN0) and N
 *				(AIN3) input.
 */
esp_err_t ads101x_read_differential_0_3(ads101x_t *const me,
		                                    int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, ADS101X_REG_CONFIG_MUX_DIFF_0_3, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	bool conversion_is_complete = false;

	do {
		delay_us(5 * 1000); /* Wait for 5 ms */
		ads101x_conversion_complete(me, &conversion_is_complete);
	} while (!conversion_is_complete);

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the voltage difference between the P (AIN1) and N
 *				(AIN3) input.
 */
esp_err_t ads101x_read_differential_1_3(ads101x_t *const me,
		                                    int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, ADS101X_REG_CONFIG_MUX_DIFF_1_3, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	bool conversion_is_complete = false;

	do {
		delay_us(5 * 1000); /* Wait for 5 ms */
		ads101x_conversion_complete(me, &conversion_is_complete);
	} while (!conversion_is_complete);

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the voltage difference between the P (AIN2) and N
 *				(AIN3) input.
 */
esp_err_t ads101x_read_differential_2_3(ads101x_t *const me,
		                                    int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Perform a oneshot ADC reading */
	ret = ads101x_start_reading(me, ADS101X_REG_CONFIG_MUX_DIFF_2_3, ADS101X_MODE_ONESHOT);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Check if the conversion is complete */
	bool conversion_is_complete = false;

	do {
		delay_us(5 * 1000); /* Wait for 5 ms */
		ads101x_conversion_complete(me, &conversion_is_complete);
	} while (!conversion_is_complete);

	/* Get the las ADC conversion result */
	ret = ads101x_get_last_conversion_results(me, adc_result);

	if (ret != ESP_OK) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that sets up the comparator to operator in basic mode,
 *        causing the ALRT/RDY pin to assert when the ADC value exceeeds the
 *        specified value.
 */
esp_err_t ads101x_start_comparator_single_ended(ads101x_t *const me,
		                                            ads101x_channel_t channel,
																								int16_t threshold) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Fill the ADS101x configuration value */
	uint16_t config = ADS101X_REG_CONFIG_CQUE_1CONV |   /* Comparator enabled and
	                                                       asserts on 1 match */
			              ADS101X_REG_CONFIG_CLAT_LATCH |   /* Latching mode */
										ADS101X_REG_CONFIG_CPOL_ACTVLOW | /* Alert/ready active low */
										ADS101X_REG_CONFIG_CMODE_TRAD |   /* Traditional comparator */
										ADS101X_REG_CONFIG_MODE_CONTIN;   /* Continuos conversion
										                                     mode */

	/* Set PGA/voltage range */
	config |= me->gain;

	/* Set data rate */
	config |= me->data_rate;

	/* Set channel */
	config |= channel;

	/* Set the high threshold register */
	if (i2c_write(ADS101X_REG_POINTER_HITHRESH, threshold << me->bit_shift, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	/* Set the new ADC configuration */
	if (i2c_write(ADS101X_REG_POINTER_CONFIG, config, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that reads the last conversion results without changin the
 *        configuration value.
 */
esp_err_t ads101x_get_last_conversion_results(ads101x_t *const me,
		                                          int16_t *adc_result) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Read the conversion result */
	uint16_t result;

	if (i2c_read(ADS101X_REG_POINTER_CONVERT, &result, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	result >>= me->bit_shift;

	if (me->bit_shift == 0) {
		*adc_result = (int16_t)result;
	}
	else {
		/* Shift 12-bit results right 4 bits for the ADS101x,
		 * making sure we keep the sign bit intact */
		if (result > 0x07FF) {
			/* Negative number, extend the sign to 16th bit */
			result |= 0xF000;
		}

		*adc_result = (int16_t)result;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that computes the voltage value from ADC raw value
 */
float ads101x_compute_volts(ads101x_t *const me, int16_t counts) {
	float fs_range;

	switch (me->gain) {
		case ADS101X_GAIN_TWOTHIRDS:
			fs_range = 6.144f;
			break;
		case ADS101X_GAIN_ONE:
			fs_range = 4.096f;
			break;
		case ADS101X_GAIN_TWO:
			fs_range = 2.048f;
			break;
		case ADS101X_GAIN_FOUR:
			fs_range = 1.024f;
			break;
		case ADS101X_GAIN_EIGHT:
			fs_range = 0.512f;
			break;
		case ADS101X_GAIN_SIXTEEN:
			fs_range = 0.256f;
			break;
		default:
			fs_range = 0.0f;
	}

	return (counts * (fs_range / (32768 >> me->bit_shift)));
}


/**
 * @brief Function that sets the ADS101x gain
 */
void ads101x_set_gain(ads101x_t *const me, ads101x_gain_t gain) {
	me->gain = gain;
}

/**
 * @brief Function that get the ADS101x gain
 */
ads101x_gain_t ads101x_get_gain(ads101x_t *const me) {
	return me->gain;
}

/**
 * @brief Function that sets the ADS101x data rate
 */
void ads101x_set_data_rate(ads101x_t *const me, ads101x_data_rate_t data_rate) {
	me->data_rate = data_rate;
}

/**
 * @brief Function that gets ADS101x data rate
 */
ads101x_data_rate_t ads101x_get_data_rate(ads101x_t *const me) {
	return me->data_rate;
}

/**
 * @brief Function that stars the conversion function
 */
esp_err_t ads101x_start_reading(ads101x_t *const me, uint16_t mux,
		                            ads101x_mode_t mode) {
	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Fill the ADS101x configuration value */
	uint16_t config = ADS101X_REG_CONFIG_CQUE_1CONV |   /* Set CQUE to any value
	                                                       other than None so we
	                                                       can use it in RDY mode
	                                                       */
			              ADS101X_REG_CONFIG_CLAT_NONLAT |  /* Non-latching */
										ADS101X_REG_CONFIG_CPOL_ACTVLOW | /* ALERT/RDY active low */
										ADS101X_REG_CONFIG_CMODE_TRAD;    /* Traditional comparator */

	/* Configure the reading mode */
	if (mode == ADS101X_MODE_CONTINUOS) {
		config |= ADS101X_REG_CONFIG_MODE_CONTIN;
	}
	else {
		config |= ADS101X_REG_CONFIG_MODE_CONTIN;
	}

	/* Set PGA/voltage range */
	config |= me->gain;

	/* Set data rate */
	config |= me->data_rate;

	/* Set channel */
	config |= mux;

	/* Set to start a single-conversion */
	config |= ADS101X_REG_CONFIG_OS_SINGLE;

	/* Set the new ADC configuration */
	if (i2c_write(ADS101X_REG_POINTER_CONFIG, config, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	/* Set ALERT/RDY to RDY mode */
	if (i2c_write(ADS101X_REG_POINTER_HITHRESH, 0x8000, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	if (i2c_write(ADS101X_REG_POINTER_HITHRESH, 0x0000, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	/* Return ESP_OK */
	return ret;
}

/**
 * @brief Function that check if the ADC reading is complete
 */
esp_err_t ads101x_conversion_complete(ads101x_t *const me, bool *is_complete) {

	/* Variable to return error code */
	esp_err_t ret = ESP_OK;

	/* Check if the device is performing a conversion */
	uint16_t rx_data = 0;

	if (i2c_read(ADS101X_REG_POINTER_CONFIG & 0x8000, &rx_data, me->i2c_dev) < 0) {
		return ESP_FAIL;
	}

	*is_complete = (bool)rx_data;

	/* Return ESP_OK */
	return ret;
}

/* Private function definitions ----------------------------------------------*/

/**
 * @brief Function that implements the default I2C read transaction
 */
static int8_t i2c_read(uint8_t reg_addr, uint16_t *reg_data, void *intf) {
	int8_t ret = 0;
	i2c_bus_dev_t *dev = (i2c_bus_dev_t *)intf;

	uint8_t buf[2] = {0};

	ret =  dev->read(reg_addr ? &reg_addr : NULL, reg_addr ? 1 : 0, buf, 2, dev);

	if (ret < 0) {
		return ret;
	}

	*reg_data = (uint16_t)((buf[0] << 8) | buf[1]);

	/* Return for success */
	return ret;
}
/**
 * @brief Function that implements the default I2C write transaction
 */
static int8_t i2c_write(uint8_t reg_addr, const uint16_t reg_data,
		                    void *intf) {
	int8_t ret = 0;
	i2c_bus_dev_t *dev = (i2c_bus_dev_t *)intf;

	uint8_t buf[2] = {(uint8_t)((reg_data & 0xFF00) >> 8),
			(uint8_t)(reg_data & 0x00FF)};

	ret = dev->write(&reg_addr, 1, buf, 2, dev);

	if (ret < 0) {
		return ret;
	}

	/* Return for success */
	return ret;
}
/**
 * @brief Function that implements a micro seconds delay
 */
static void delay_us(uint32_t period_us) {
	uint64_t m = (uint64_t)esp_timer_get_time();

  if (period_us) {
  	uint64_t e = (m + period_us);

  	if (m > e) { /* overflow */
  		while ((uint64_t)esp_timer_get_time() > e) {
  			NOP();
  		}
  	}

  	while ((uint64_t)esp_timer_get_time() < e) {
  		NOP();
  	}
  }
}

/***************************** END OF FILE ************************************/
