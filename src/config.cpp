#include "config.h"
#include "EEPROM.h"

#define LORAWAN_KEYS_ADDRESS 0x0
#define CONFIG_ADDRESS 0x20

lorawan_keys_t lorawan_keys;
device_config_t device_config;

int read_device_config() {
    EEPROM.get(CONFIG_ADDRESS, device_config);

    if (device_config.version_config != DEVICE_CONFIG_VERSION)
        return -1;
    else
        return 0;
}

void write_device_config(device_config_t &device_config_new) {
    if (memcmp(&device_config, &device_config_new, sizeof(device_config_t))) {
        device_config = device_config_new;
        EEPROM.put(CONFIG_ADDRESS, device_config);
        log_warning_ln(F("Settings written"));
    } else {
        log_warning_ln(F("Settings unchanged"));
    }
}

void init_device_config(bool reset) {
    EEPROM.get(LORAWAN_KEYS_ADDRESS, lorawan_keys);
    bool not_valid_seeting = read_device_config();
    UNUSED(not_valid_seeting);
    device_config_t device_config_new = device_config;
#ifndef RESET_SETTINGS
    if (reset || not_valid_seeting )
#endif
    {
        device_config_new.version_config = DEVICE_CONFIG_VERSION;
        device_config_new.version = DEVICE_VERSION;
        device_config_new.measure_interval_s = TX_INTERVAL;
        device_config_new.measure_average = DEFAULT_AVERAGE_MEASUREMENTS;
        device_config_new.max_skiped_measurements = DEFAULT_MAX_SKIPED_MEASUREMENTS;
        set_device_specific_config(device_config_new.device);
    }
    device_config_new.version = DEVICE_VERSION;
    write_device_config(device_config_new);

}

void print_hex_mem(uint8_t *mem, size_t len, bool bigEndian=true) {
    log_info   (F("0x"));
    for (size_t i=0; i < len; i++) {
        size_t ii = bigEndian ? i : len-(i+1);
        uint8_t byte = *(mem + ii);
        if (byte < 16)
            log_info('0');
        log_info (byte, HEX);
    }
    log_info_ln("");
}

void print_buildinfo() {
    log_info(F("DEVICE VERSION: "));
        log_info_ln(DEVICE_VERSION);
    log_info(F("DEVICE CONFIG: "));
        print_hex_mem((uint8_t*)&device_config, sizeof(device_config));
    log_info(F("Device UID: "));
        print_hex_mem((uint8_t*)UID_BASE, UID_BYTES_LENGTH);
    log_info(F("BUILD DATE: "));
        log_info_ln(__DATE__);
    log_info(F("BUILD TIME: "));
        log_info_ln(__TIME__);
    log_info(F("COMMIT ID: "));
        log_info_ln(GIT_COMMIT_ID);
    log_info(F("APPEUI: "));
        print_hex_mem((uint8_t*)&lorawan_keys.appeui, 8, false);
    log_info(F("DEVEUI: "));
        print_hex_mem((uint8_t*)&lorawan_keys.deveui, 8, false);
}