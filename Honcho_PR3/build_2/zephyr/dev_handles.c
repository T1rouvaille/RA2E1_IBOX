#include <device.h>
#include <toolchain.h>

/* 1 : /soc/clock@40000000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_clock_40000000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 2 : /soc/gpio@50000000:
 * Direct Dependencies:
 *   - (/soc)
 * Supported:
 *   - (/enables/BattTH_EN)
 *   - (/enables/LDO_EN)
 *   - (/enables/LaserBuck_EN)
 *   - (/enables/LaserP2bias)
 *   - (/enables/Laser_Current)
 *   - (/enables/Laser_PS_ADC)
 *   - (/enables/PACK_ID_EN)
 *   - (/enables/Uart_RX_29)
 *   - (/enables/Vbatpack_EN)
 *   - (/enables/Vbatt_coin_EN)
 *   - (/enables/Vbatt_coin_adc)
 *   - (/interrupt/keypadint)
 *   - (/interrupt/pendulumlock)
 *   - (/interrupt/v3p3detect)
 *   - (/interrupt/yawfault)
 *   - (/leds/Blue_led)
 *   - (/leds/Drop_led)
 *   - (/leds/soc1)
 *   - (/leds/soc2)
 *   - (/leds/soc3)
 *   - (/outputs/Uart_TX_28)
 *   - /soc/spi@40004000
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_gpio_50000000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, 15, DEVICE_HANDLE_ENDS };

/* 3 : /soc/gpio@50000300:
 * Direct Dependencies:
 *   - (/soc)
 * Supported:
 *   - (/enables/LaserLbias)
 *   - (/enables/LaserP1bias)
 *   - (/enables/MotorBuck_EN)
 *   - (/interrupt/vertfault)
 *   - (/leds/Slope_led)
 *   - (/outputs/motor_dir)
 *   - (/outputs/motor_reset)
 *   - (/outputs/motor_vertEN)
 *   - (/outputs/motor_yawEN)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_gpio_50000300[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 4 : /soc/watchdog@40010000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_watchdog_40010000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 5 : /soc/random@4000d000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_random_4000d000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 6 : /soc/crypto@5002a000:
 * Direct Dependencies:
 *   - (/soc)
 * Supported:
 *   - (/soc/crypto@5002a000/crypto@5002b000)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_crypto_5002a000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 7 : /soc/uart@40028000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/pin-controller/uart1_default)
 *   - (/pin-controller/uart1_sleep)
 *   - (/soc/interrupt-controller@e000e100)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_uart_40028000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 8 : /soc/adc@40007000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/soc/interrupt-controller@e000e100)
 * Supported:
 *   - (/zephyr,user)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_adc_40007000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 9 : /soc/i2c@40003000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/pin-controller/i2c0_default)
 *   - (/pin-controller/i2c0_sleep)
 *   - (/soc/interrupt-controller@e000e100)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_i2c_40003000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 10 : /soc/pwm@4002d000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/pin-controller/pwm3_default)
 *   - (/pin-controller/pwm3_sleep)
 *   - (/soc/interrupt-controller@e000e100)
 * Supported:
 *   - (/pwm/vert_motor_step)
 *   - (/pwm/yaw_motor_step)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pwm_4002d000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 11 : /soc/pwm@40022000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/pin-controller/pwm2_default)
 *   - (/pin-controller/pwm2_sleep)
 *   - (/soc/interrupt-controller@e000e100)
 * Supported:
 *   - (/pwm/level_pwm)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pwm_40022000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 12 : /soc/pwm@40021000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/pin-controller/pwm1_default)
 *   - (/pin-controller/pwm1_sleep)
 *   - (/soc/interrupt-controller@e000e100)
 * Supported:
 *   - (/pwm/plum2_pwm)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pwm_40021000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 13 : /soc/pwm@4001c000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/pin-controller/pwm0_default)
 *   - (/pin-controller/pwm0_sleep)
 *   - (/soc/interrupt-controller@e000e100)
 * Supported:
 *   - (/pwm/plum1_pwm)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_pwm_4001c000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 14 : /soc/flash-controller@4001e000:
 * Direct Dependencies:
 *   - (/soc)
 * Supported:
 *   - (/soc/flash-controller@4001e000/flash@0)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_flash_controller_4001e000[] = { DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };

/* 15 : /soc/spi@40004000:
 * Direct Dependencies:
 *   - (/soc)
 *   - (/pin-controller/spi1_default)
 *   - (/pin-controller/spi1_sleep)
 *   - /soc/gpio@50000000
 *   - (/soc/interrupt-controller@e000e100)
 */
const device_handle_t __aligned(2) __attribute__((__section__(".__device_handles_pass2")))
__devicehdl_DT_N_S_soc_S_spi_40004000[] = { 2, DEVICE_HANDLE_SEP, DEVICE_HANDLE_SEP, DEVICE_HANDLE_ENDS };
