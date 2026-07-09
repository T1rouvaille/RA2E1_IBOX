
#include "custom_adc.h"

// ###########################################
// # GLOBAL VAR
// ###########################################
static int32_t adc_vref =0;
static uint8_t avg_cnt = 0;
volatile uint8_t soc_reg = 0;
static int16_t pack_sample_buffer[PACKADC_NUM_CHANNELS];
static int16_t lasercurrent_sample_buffer[1];
static volatile uint32_t vbatt = 0, th = 0;
static volatile int32_t lscurrent = 0;
static float  ntc =0;
static volatile uint32_t vbatt_avg = 0, th_avg = 0, c3_avg= 0;
volatile uint16_t adc_thread_sleep_time = 10; //in seconds



/* Get the numbers of up to two channels */
static uint8_t pack_channel_ids[PACKADC_NUM_CHANNELS] = {
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 0),
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 1)
	// DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 2)
};
static uint8_t lasercurrent_channel_ids[1]= {
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 2)
};


static struct adc_channel_cfg pack_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	/* channel ID will be overwritten below */
	.channel_id = 0,
	.differential = 0
};

static struct adc_sequence pack_sequence = {
	/* individual channels will be added below */
	.channels    = 0,
	.buffer      = pack_sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(pack_sample_buffer),
	.resolution  = ADC_RESOLUTION,
};

static struct adc_channel_cfg lasercurrent_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	/* channel ID will be overwritten below */
	.channel_id = 0,
	.differential = 0
};

static struct adc_sequence lasercurrent_sequence = {
	/* individual channels will be added below */
	.channels    = 0,
	.buffer      = lasercurrent_sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(lasercurrent_sample_buffer),
	.resolution  = ADC_RESOLUTION,
};

/**
 * @brief custom_adc_init
 * adc init fucntion to initialize the adc drivers and setup buffers for fo DMA read
 * 
 */
void custom_adc_init()
{
    int err;
	
    lowvoltage_flag = false;

	if (!device_is_ready(dev_adc)) {
		printk("ADC device not found\n");
		return;
	}

	/*
	 * Configure channels individually prior to sampling
	 */
	for (uint8_t i = 0; i < PACKADC_NUM_CHANNELS; i++)
    {
		pack_channel_cfg.channel_id = pack_channel_ids[i];
    #ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
            pack_channel_cfg.input_positive = ADC_INPUT_POS_OFFSET + pack_channel_ids[i];
    #endif

		adc_channel_setup(dev_adc, &pack_channel_cfg);

		pack_sequence.channels |= BIT(pack_channel_ids[i]);
	}

    lasercurrent_channel_cfg.channel_id = lasercurrent_channel_ids[0];
    #ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
            lasercurrent_channel_cfg.input_positive = ADC_INPUT_POS_OFFSET + lasercurrent_channel_ids[0];
    #endif

		adc_channel_setup(dev_adc, &lasercurrent_channel_cfg);

		lasercurrent_sequence.channels |= BIT(lasercurrent_channel_ids[0]);

    // channel_cfg1.channel_id = channel_ids1[0];
    // #ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
    //         channel_cfg1.input_positive = ADC_INPUT_POS_OFFSET + channel_ids1[0];
    // #endif

	// 	adc_channel_setup(dev_adc, &channel_cfg1);

	// 	sequence1.channels |= BIT(channel_ids1[0]);

	adc_vref = adc_ref_internal(dev_adc);

    err = gpio_pin_configure_dt(&Vbaten, GPIO_OUTPUT_LOW);		//here set initial state
	if (err != 0)
	{
		return;
	}

    err = gpio_pin_set_dt(&Vbaten, 1);

    
    err = gpio_pin_configure_dt(&Battthen, GPIO_OUTPUT_LOW);		//here set initial state
	if (err != 0)
	{
		return;
	}

    err = gpio_pin_set_dt(&Battthen, 1);

    //  err = gpio_pin_configure_dt(&uartrx, NRF_GPIO_PIN_NOPULL);

}

/**
 * @brief disable_battmeasurement
 * helper fucntion to disable Pack and TH adc measurment to save battery
 * 
 */
void disable_battmeasurement()
{
    int err;
    err = gpio_pin_set_dt(&Vbaten, 0);
    err = gpio_pin_set_dt(&Battthen, 0);

}

/**
 * @brief helper function to change the timing of adc thread. 
 *
 * @param x new time in ms 
 */
void adc_thread_time(int x)
{
    adc_thread_sleep_time = x;
}

/**
 * @brief lasercurrent_limitor
 * Function that takes care of the current limit on each of the laser diodes in each of the configration.
 *  Level  350mA
 *  Plumb1 300mA 
 *  Plumb2 300mA 
 *  Level+Plumb1 650mA 
 *  Level+Plumb2 650mA 
 *  Plumb1 + Plumb2 600mA 
 *  Level+plumb1+plumb2 950mA
 * 
 * @param var laser diode current adc value
 */
static void lasercurrent_limitor(int32_t var)
{
    uint8_t x = laser_read_status();
    // printk("Entered here %d\n", x);
    //Add Current shutdown in datalog.
    switch(x)
    {
        case 0x01 : 
            if(var >= PLUMB2_LIMIT)
            {
                printk(" Plumb2 current over limit \n");
                DL_Inc_U32(&DL_log.shutdown_8V_buck_upper_limit);
                all_laser_off(); 
            }
            break;   

        case 0x02 :
            if(var >= PLUMB1_LIMIT)
            {
                printk(" Plumb1 current over limit \n");
                DL_Inc_U32(&DL_log.shutdown_8V_buck_upper_limit);
                all_laser_off(); 
            } 
            break;

        case 0x03 :
            if(var >= (PLUMB1_LIMIT + PLUMB2_LIMIT))
            {
                printk(" Plumb2 + Plumb1 current over limit \n");
                DL_Inc_U32(&DL_log.shutdown_8V_buck_upper_limit);
                all_laser_off(); 
            } 
            break;

        case 0x04 :
            if(var >= LEVEL_LIMIT)
            {
                printk(" Level current over limit \n");
                DL_Inc_U32(&DL_log.shutdown_8V_buck_upper_limit);
                all_laser_off(); 
            } 
            break;

        case 0x05 :
            if(var >= (LEVEL_LIMIT + PLUMB1_LIMIT))
            {
                printk(" Level + plumb1 current over limit \n");
                DL_Inc_U32(&DL_log.shutdown_8V_buck_upper_limit);
                all_laser_off(); 
            } 
            break;

        case 0x06 :
            if(var >= (LEVEL_LIMIT + PLUMB2_LIMIT))
            {
                printk(" Level + plumb2 current over limit \n");
                DL_Inc_U32(&DL_log.shutdown_8V_buck_upper_limit);
                all_laser_off(); 
            } 
            break;

        case 0x07 : 
            if(var >= (LEVEL_LIMIT + PLUMB1_LIMIT + PLUMB2_LIMIT))
            {
                printk(" All laser current over limit \n");
                DL_Inc_U32(&DL_log.shutdown_8V_buck_upper_limit);
                all_laser_off(); 
            } 
            break;
        default :
            break;
    }
}

/**
 * @brief lasercurrent_adc_read
 * This is the entry point for laser_current_read_thread
 * takes an avg of 16 adc values and calls the lasercurrent_limitor api
 * 
 */
void lasercurrent_adc_read()
{
    while(1)
    {
        uint8_t x = laser_read_status();
        if(x!=0 && pendblink_flag==0)
        {
            for(int i =0; i<16; i++)
                {
                    int err = adc_read(dev_adc, &lasercurrent_sequence);
                        if (err != 0) {
                            printk("ADC reading failed with error %d.\n", err);
                            return;
                        }

                        lscurrent += lasercurrent_sample_buffer[0];
                        k_busy_wait(500);
                }
                lscurrent = lscurrent >>4;
                lasercurrent_limitor(lscurrent);
                // printk("Laser reading:");
                // printk(" %d",lscurrent);
                // printk("\n");
                
        }
        
        k_sleep(K_SECONDS(1));
    }
}

/**
 * @brief pack adc read
 * Thread entry point for the Pack read adc thread. 
 * Raed the vcc and Th adc value , avg over 16 samples
 * the adc value is passed to soc led manager adn th sample is used 
 * to back calculate the resistance 
 * 10k -> normal operation
 * 2.2k -> High volatge 
 * 
 */
void pack_adc_read()
{
    int err;
    
    while(1)
    {
        
        // printk("pin turned high for volatge measurement\n");
        vbatt = 0, th = 0 ;
        for(int i =0; i<16; i++)
        {
            err = adc_read(dev_adc, &pack_sequence);
                if (err != 0) {
                    printk("ADC reading failed with error %d.\n", err);
                    return;
                }

                
                vbatt += pack_sample_buffer[0];
                th += pack_sample_buffer[1];
                k_busy_wait(500);
        }
            vbatt = vbatt>>4;
            th = th>>4;


            if (vbatt > BATT_VOLATGE_75)                   
            {
                soc_reg = 0x16;
            }
            else if(vbatt > BATT_VOLATGE_50)              
            {
                soc_reg = 0x08;
            }
            else if(vbatt >BATT_VOLATGE_25)                
            {
                soc_reg = 0x04;
            }
            else if(vbatt > BATT_VOLATGE_10)               
            {
                soc_reg = 0x02;
            }
            else 
            {
                soc_reg = 1;
            }

            ntc = (float)(vbatt*11.84 / th) -8.83;      //Refer Notes in the bitbucket library

            // printk("ADC reading:");
            // printk(" %u %u ", vbatt, th);
            // printk("\n");

            if (ntc < 2.3)
            {
                //Do a Datalog write to record a Hot pack shutdown;
                DL_Inc_U32(&DL_log.shutdown_ntc_hot_pack);
                k_sem_give(&datalogwrite_semaphore);
                k_sleep(K_USEC(500));
                sys_off();

            }
            if(power_on)
            {    
                socled_manager(soc_reg);
            }

        k_sleep(K_SECONDS(1));

    }
}

// ###########################################
// # THREAD DECLERATION
// ###########################################


K_THREAD_DEFINE(LASER_CURRENT_READ_THREAD, LASERCURRENT_THREAD_STACK_SIZE, lasercurrent_adc_read, NULL, NULL,
NULL, LASERCURRENT_THREAD_PRIORITY, 0, -1);
K_THREAD_DEFINE(PACK_ADC_READ_THREAD, ADCREAD_THREAD_STACK_SIZE, pack_adc_read, NULL, NULL,
				NULL, ADCREAD_THREAD_PRIORITY, 0, -1);

