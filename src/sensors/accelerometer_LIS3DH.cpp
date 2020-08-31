#include "config.h"
#ifdef SENSOR_ACCELEROMETER_LIS3DH

#include "sensor.h"
#include "sleep.h"
#include "lis3dh_reg.h"
#include "Wire.h"
#include "assert.h"

static const uint8_t LIS3DH_I2C_ADD = 0x19;
static const uint8_t DEFAULT_MIN_ACCELERATION_DG_2_SEND=10; //1g

class Sensor_accelerometer: Sensor{
    private:
        void set_config(device_config_device_t& specific_device_config) override;
        void init (bool firstTime) override;
        bool measure_intern() override;
        void send(CayenneLPP& lpp) override;
        //void stop () override;

        stmdev_ctx_t dev_ctx;

        struct acceleration_t {float x;float y;float z;};
        acceleration_t acceleration = {0};
        acceleration_t old_acceleration = {0};

        bool is_moving;
        bool old_is_moving;

};
static Sensor_accelerometer sensor;

void Sensor_accelerometer::set_config(device_config_device_t& specific_device_config) {
    specific_device_config.min_acceleration_dg_2_send = DEFAULT_MIN_ACCELERATION_DG_2_SEND;
}



static int32_t platform_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len){
    reg |= 0x80; //turn auto-increment bit on, bit 7 for I2C
    Wire.beginTransmission(LIS3DH_I2C_ADD);
    Wire.write(reg);                   // sends instruction byte  
    Wire.write(bufp, len);             // sends buffer  
    return Wire.endTransmission();
    return 0;
}
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len){
    Wire.beginTransmission(LIS3DH_I2C_ADD);
    reg |= 0x80; //turn auto-increment bit on, bit 7 for I2C
    Wire.write(reg);
    if( Wire.endTransmission() != 0 )
    {
        return -1;
    }
    else  //OK, all worked, keep going
    {
        Wire.requestFrom((uint8_t)LIS3DH_I2C_ADD, (uint8_t)len);
        for (size_t i=0; i< len && Wire.available(); i++)
        {
            bufp[i] = Wire.read(); // receive a byte as character
        }
    }
    return Wire.endTransmission();
}

void Sensor_accelerometer::init(bool firstTime){
    
    if (firstTime) {
        
		//Pull-up to select I2c mode
        pinMode(PIN_ACCEL_SDO, INPUT_PULLUP);
        
        /*
        *  Initialize mems driver interface
        */
        lis3dh_int1_cfg_t int1_cfg;
        lis3dh_ctrl_reg3_t ctrl_reg3;
        uint8_t dummy;

        dev_ctx.write_reg = platform_write;
        dev_ctx.read_reg = platform_read;
        //dev_ctx.handle = &I2C1;

        /*
        *  Check device ID
        */
        uint8_t whoamI;
        lis3dh_device_id_get(&dev_ctx, &whoamI);
        if (whoamI != LIS3DH_ID)
        {
            log_error_ln(F("Failed to init accelerometer LIS3DH"));
            while (1) yield();
        }
        log_info_ln("Accelerometer LIS3DH found!");

        /*
        * High-pass filter enabled on interrupt activity 1
        */
        assert(lis3dh_high_pass_int_conf_set(&dev_ctx, LIS3DH_ON_INT1_GEN) == 0);

        /*
        * Enable HP filter for wake-up event detection
        *
        * Use this setting to remove gravity on data output
        */
        assert(lis3dh_high_pass_on_outputs_set(&dev_ctx, PROPERTY_ENABLE) == 0);

        /*
        * Enable AOI1 on int1 pin
        */
        assert(lis3dh_pin_int1_config_get(&dev_ctx, &ctrl_reg3) == 0);
        ctrl_reg3.i1_ia1 = PROPERTY_ENABLE;
        assert(lis3dh_pin_int1_config_set(&dev_ctx, &ctrl_reg3) == 0);

        /*
        * Interrupt 1 pin latched
        */
        assert(lis3dh_int1_pin_notification_mode_set(&dev_ctx,
                            LIS3DH_INT1_LATCHED) == 0);

        /*
        * Set full scale to 2 g
        */
        assert(lis3dh_full_scale_set(&dev_ctx, LIS3DH_2g) == 0);

        /*
        * Set interrupt threshold to 0x10 -> 250 mg
        */
        assert(lis3dh_int1_gen_threshold_set(&dev_ctx, 0x10) == 0);

        /*
        * Set no time duration
        */
        assert(lis3dh_int1_gen_duration_set(&dev_ctx, 0) == 0);

        /*
        * Dummy read to force the HP filter to current acceleration value.
        */
        assert(lis3dh_filter_reference_get(&dev_ctx, &dummy) == 0);

        /*
        * Configure wake-up interrupt event on all axis
        */
        assert(lis3dh_int1_gen_conf_get(&dev_ctx, &int1_cfg) == 0);
        int1_cfg.zhie = PROPERTY_ENABLE;
        int1_cfg.yhie = PROPERTY_ENABLE;
        int1_cfg.xhie = PROPERTY_ENABLE;
        int1_cfg.aoi = PROPERTY_DISABLE;
        assert(lis3dh_int1_gen_conf_set(&dev_ctx, &int1_cfg) == 0);

        /*
        * Set device in LP mode
        */
        assert(lis3dh_operating_mode_set(&dev_ctx, LIS3DH_LP_8bit) == 0);

        /*
        * Set Output Data Rate to 100 Hz
        */
        assert(lis3dh_data_rate_set(&dev_ctx, LIS3DH_ODR_100Hz) == 0);

    }
}
void Sensor_accelerometer_interrupt() { /*Nothing to do - waking up is enough */}
bool Sensor_accelerometer::measure_intern() {

    lis3dh_int1_src_t src;
    

    /*
     * Read INT pin 1 in polling mode
     * or read src status register
     */
    assert(lis3dh_int1_gen_source_get(&dev_ctx, &src) == 0);
    if (src.xh || src.yh || src.zh)
    {

        //Moving
        is_moving = true;

        log_info("Movement detected:");
        log_info("\tX: "); log_info(src.xh);
        log_info("\tY: "); log_info(src.yh);
        log_info("\tZ: "); log_info(src.zh);
        log_info_ln("");

        //Enter fast mode
        fast_sleep(TX_FAST_INTERVAL_S * 1000);

        //Dissable interrupt to avoid waking up between
        detachInterrupt(digitalPinToInterrupt(PIN_ACCEL_INT1));
        
        // Disable AOI1 on int1 pin
        // NOTE: without this the CPU wakes up even if we disabled the interrupt and afterwards consumption is 400 uA
        lis3dh_ctrl_reg3_t ctrl_reg3;
        assert(lis3dh_pin_int1_config_get(&dev_ctx, &ctrl_reg3) == 0);
        ctrl_reg3.i1_ia1 = PROPERTY_DISABLE;
        assert(lis3dh_pin_int1_config_set(&dev_ctx, &ctrl_reg3) == 0);

        
        
    } else {
        //no move - do not enable fast sleep
        is_moving = false;

        //Enable interrupt
        attachInterrupt(digitalPinToInterrupt(PIN_ACCEL_INT1), Sensor_accelerometer_interrupt, RISING);
        
        // Enable AOI1 on int1 pin
        lis3dh_ctrl_reg3_t ctrl_reg3;
        assert(lis3dh_pin_int1_config_get(&dev_ctx, &ctrl_reg3) == 0);
        ctrl_reg3.i1_ia1 = PROPERTY_ENABLE;
        assert(lis3dh_pin_int1_config_set(&dev_ctx, &ctrl_reg3) == 0);
    }

    log_debug("Is moving: ");log_debug_ln(is_moving);
    
    // Reset high pass filter
    assert(lis3dh_high_pass_on_outputs_set(&dev_ctx, PROPERTY_DISABLE) == 0);

    //Set device in HR mode
    assert(lis3dh_operating_mode_set(&dev_ctx, LIS3DH_LP_8bit) == 0);  
   
    /* Read accelerometer data to ensure filtered value is removed*/
    uint16_t data_raw_acceleration[3] = {0};
    assert(lis3dh_acceleration_raw_get(&dev_ctx, (uint8_t*)data_raw_acceleration) == 0);
    

     /* Read output until new value available */
    uint8_t data_ready=0;
    while(!data_ready) assert(lis3dh_xl_data_ready_get(&dev_ctx, &data_ready) == 0);
    
    /* Read accelerometer data - this time not filtered */
    assert(lis3dh_acceleration_raw_get(&dev_ctx, (uint8_t*)data_raw_acceleration) == 0);

    // Covert data to g
    acceleration.x = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[0])/1000;
    acceleration.y = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[1])/1000;
    acceleration.z = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[2])/1000;

    log_debug("Acceleration:");
    log_debug("  X: "); log_debug(acceleration.x);
    log_debug("  Y: "); log_debug(acceleration.y);
    log_debug("  Z: "); log_debug(acceleration.z);
    log_debug_ln("  g");

    //Set device in HR mode
    assert(lis3dh_operating_mode_set(&dev_ctx, LIS3DH_LP_8bit) == 0);  
   /*
    * Enable HP filter for wake-up event detection
    *
    * Use this setting to remove gravity on data output
    */
    assert(lis3dh_high_pass_on_outputs_set(&dev_ctx, PROPERTY_ENABLE) == 0);

    //Reset interrupt by reading the regiter
    while (src.xh || src.yh || src.zh) assert(lis3dh_int1_gen_source_get(&dev_ctx, &src) == 0);

    //Find if it a value has changed enough
    return (
      (old_is_moving != is_moving) ||
      ( !is_moving && (
        (abs(acceleration.x - old_acceleration.x) >= device_config.device.min_acceleration_dg_2_send) ||
        (abs(acceleration.y - old_acceleration.y) >= device_config.device.min_acceleration_dg_2_send) ||
        (abs(acceleration.z - old_acceleration.z) >= device_config.device.min_acceleration_dg_2_send)
      ))
    );

}

void Sensor_accelerometer::send(CayenneLPP& lpp) {

    
    //Add measurements and remember last transmit
    lpp.addAccelerometer(SENSOR_ACCELEROMETER_CHANNEL, acceleration.x, acceleration.y, acceleration.z);
    old_acceleration = acceleration;
    lpp.addDigitalInput(SENSOR_MOVING_CHANNEL, is_moving);
    old_is_moving = is_moving;

}

#endif //SENSOR_ACCELEROMETER_LIS3DH