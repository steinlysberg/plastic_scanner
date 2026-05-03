#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>

#define LED_ON 1
#define LED_OFF 0

//For LED-driver:
#define TLC_NODE DT_NODELABEL(tlc59108)
static const struct i2c_dt_spec led_driver_i2c = I2C_DT_SPEC_GET(TLC_NODE);

//For ADC:
#define NAU_NODE DT_NODELABEL(nau7802)
static const struct i2c_dt_spec adc_i2c = I2C_DT_SPEC_GET(NAU_NODE);

// Set up LED driver:
uint8_t tlc_setup(){

    if (!device_is_ready(led_driver_i2c.bus)) {
        printk("LED driver I2C bus %s is not ready!\n\r",led_driver_i2c.bus->name);
        return -1;
    }
    
    // Default is low power mode. Enable normal mode.
    uint8_t ret = i2c_reg_write_byte_dt(&led_driver_i2c, 0x00, 0x00); 
    if (ret != 0){
        printk("Failed to write to TLC.\r\n");
        return -1;
    }

    return 0;
}

// Set up ADC:
uint8_t nau_setup(){
    if (!device_is_ready(adc_i2c.bus)) {
        printk("ADC I2C bus %s is not ready!\n\r",adc_i2c.bus->name);
        return -1;
    }

    //Set RR bit high to reset all registers
    uint8_t ret = i2c_reg_write_byte_dt(&adc_i2c, 0x00, 0x01); 
    if (ret != 0){
        printk("Failed to write to NAU.\r\n");
        return -1;
    }

    //Set RR bit back to low, set PUD and PUA bit to 1 to power up
    ret = i2c_reg_write_byte_dt(&adc_i2c, 0x00, 0x06); 
    if (ret != 0){
        printk("Failed to write to NAU.\r\n");
        return -1;
    }
    
    //Stall until power-up is done
    uint8_t value = 0;
    do {
        i2c_reg_read_byte_dt(&adc_i2c, 0x00, &value);
    } while (value != 0x0E);

    //Set gain to 128, disable LDO
    ret = i2c_reg_write_byte_dt(&adc_i2c, 0x01, 0x07); 
    if (ret != 0){
        printk("Failed to write to NAU.\r\n");
        return -1;
    }

    //Set sample rate to 10 SPS
    ret = i2c_reg_write_byte_dt(&adc_i2c, 0x02, 0x00); 
    if (ret != 0){
        printk("Failed to write to NAU.\r\n");
        return -1;
    }

    return 0;
}

uint8_t calibrate_adc(){

    //Start internal offset calibration
    uint8_t ret = i2c_reg_write_byte_dt(&adc_i2c, 0x02, 0x04);
    if (ret != 0){
        printk("Failed to write to NAU.\r\n");
        return -1;
    }

    //Stall until internal offset calibration finishes
    uint8_t value = 0;
    do {
        i2c_reg_read_byte_dt(&adc_i2c, 0x02, &value);
        if(value & 0x08){
            printk("Calibration error (offset internal).\n\r");
            return -1;
        }
    } while(value != 0x00);


    //Start system offset calibration
    ret = i2c_reg_write_byte_dt(&adc_i2c, 0x02, 0x06);
    if (ret != 0){
        printk("Failed to write to NAU.\r\n");
        return -1;
    }

    //Stall until calibration finishes
    do {
        i2c_reg_read_byte_dt(&adc_i2c, 0x02, &value);
        if(value & 0x08){
            printk("Calibration error (offset system).\n\r");
            return -1;
        }
    } while(value != 0x02);
    
   
    //    This is system gain calibration.
    //    It has never finished without error.

    /*
    //Start system gain calibration
    ret = i2c_reg_write_byte_dt(&adc_i2c, 0x02, 0x07);
    if (ret != 0){
        printk("Failed to write to NAU.\r\n");
        return -1;
    }

    // Stall until calibration finishes
    do {
        i2c_reg_read_byte_dt(&adc_i2c, 0x02, &value);
        if(value & 0x08){
            printk("Calibration error (gain system).\n\r");
            return -1;
        }
    } while(value != 0x03);
    */
   return 0;
}

// Print to draw graph with a script:
void print_machine_readable(int32_t results[6]){
    printk("%x\n", 0xffff);
    for(uint8_t i = 0; i < 6; i++){
        printk("%i\n", results[i]);
    }
}

// Print to read results directly in terminal:
void print_human_readable(int32_t results[6]){
    printk("\n\r");
    for(uint8_t i = 0; i < 6; i++){
        printk("%i\t", results[i]);
    }
}

// Read from ADC:
int32_t get_adc_value(){
    uint8_t adc_value;
    uint8_t register_readout = 0;
    uint32_t unsigned_reading = 0;

    // Stall until a conversion is ready
    do {
        i2c_reg_read_byte_dt(&adc_i2c, 0x00, &register_readout);
    } while(!(register_readout & 0x20));

    // Read from the three ADC_OUT-registers. Merge readings into a 32 bit unsigned int.
    for(uint8_t i = 0; i < 3; i++){
        i2c_reg_read_byte_dt(&adc_i2c, 0x12 + i, &adc_value);

        unsigned_reading <<= 8;
        unsigned_reading += adc_value;
    }

    // Check if value should be negative
    // Set the first 8 bits high if the value should be negative
    if (unsigned_reading & 0x00800000){
        unsigned_reading += 0xFF000000;
    }

    return (int32_t)unsigned_reading;
}

// Turn LEDs on or off
uint8_t led_control(uint8_t led_number, uint8_t led_state){

    uint8_t reg;
    uint8_t value = 0x00;

    // Choose appropriate LED register and value for function arguments
    switch(led_number){
        case 0:
            reg = 0x0C;
            if(led_state == LED_ON){
                value = 0b00000001;
            }
            break;
        case 1:
            reg = 0x0C;
            if(led_state == LED_ON){
                value = 0b00000100;
            }
            break;
        case 2:
            reg = 0x0C;
            if(led_state == LED_ON){
                value = 0b00010000;
            }
            break;
        case 3:
            reg = 0x0C;
            if(led_state == LED_ON){
                value = 0b01000000;
            }
            break;
        case 4:
            reg = 0x0D;
            if(led_state == LED_ON){
                value = 0b00000001;
            }
            break;
        case 5:
            reg = 0x0D;
            if(led_state == LED_ON){
                value = 0b00000100;
            }
            break;
        default:
            printf("Trying to write invalid case to LEDs.\r\n");
            return -1;
    }

    // Write appropriate value to appropriate register, see that it succeeds
    uint8_t ret = i2c_reg_write_byte_dt(&led_driver_i2c, reg, value);
    if(ret != 0){
        printk("Setting LED failed.\n\r");
    } else {
        return 0;
    }
    return -1;
}

// Main, calls setup functions and then loops:
int main(void)
{   
    k_sleep(K_MSEC(2000));

    uint8_t ret = tlc_setup();
    if(ret != 0){
        printk("TLC setup failed.\n\r");
    } else {
        printk("TLC setup complete.\n\r");
    }

    ret = nau_setup();
    if(ret != 0){
        printk("NAU setup failed.\n\r");
    } else {
        printk("NAU setup complete.\n\r");
    }

    ret = calibrate_adc();
    if(ret != 0){
        printk("ADC calibration failed.\n\r");
    } else {
        printk("ADC calibration complete.\n\r");
    }

    uint32_t results[6];

	while (1) {

        //For all 6 LEDs:
        for(uint8_t i=0; i<6; i++){
            
            // LED on
            led_control(i, LED_ON);
            k_sleep(K_MSEC(10));

            //Read from ADC twice
            get_adc_value();                // Discard first reading
            results[i] = get_adc_value();   // Use second reading

            // LED off
            led_control(i, LED_OFF);
        }
        
        //Uncomment the appropriate function call:
        
        //print_machine_readable(results);
        print_human_readable(results);
	}
}
