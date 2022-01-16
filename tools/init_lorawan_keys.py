#!/usr/bin/env python3

# This tool allows to program unique lorawan keys in the last flash memory page which is then used
# in arduino as pseudo EEPROM
# It also supports setting option bytes

import subprocess
import sys
import os
import configparser
import uuid
import hashlib
import struct
import tempfile
import argparse

#Default settings matching the STM32L412KB - 128 KB Flash Cortext
UID_ADDRESS_L4=0x1FFF7590

DEVICES_LIST_FILE="config/devices.ini" #put into a subfolder so it can be put into a different git repo
PROGRAMMER="STM32_Programmer_CLI -c port=swd freq=480"
DEFAULT_FLASH_BASE=0x08000000
DEFAULT_FLASH_PAGE_SIZE=0x800


class lorawan_device:

    def __init__(self,
                 name,
                 uid_addr,
                 eeprom_address,
                 optionbytes = {},
                 flash_base=DEFAULT_FLASH_BASE,
                 flash_page_size=DEFAULT_FLASH_PAGE_SIZE):
        super().__init__()
        self.__name = name
        self.__uid_addr = uid_addr
        self.__eeprom_address = eeprom_address
        self.__optionbytes = optionbytes
        self.__flash_base = flash_base
        self.__flash_page_size = flash_page_size
    
    def program(self):

        #Read ini file - allow comments
        config = configparser.ConfigParser(comment_prefixes='/', allow_no_value=True)
        os.makedirs(os.path.dirname(DEVICES_LIST_FILE), exist_ok=True)
        config.read(DEVICES_LIST_FILE)

        #Check if the current device already known
        dev_uid = self.get_uid()
        print ("Detected device with UID "+dev_uid)

        if dev_uid not in config:
            print("Adding new device")
            config[dev_uid] = {}
        dev_config = config[dev_uid]

        #Create global section
        if "global" not in config:
            config["global"] = {}
        
        #Get the App EUI - if not set already we use a dummy so the file is created but it will not work
        global_appeui = config["global"].setdefault("appeui", hex(int(uuid.uuid4().hex,16) & 0xFFFFFFFFFFFFFFFF))

        #Store the passed device name so it can be identified in the file
        appeui = dev_config["name"] = self.__name
        
        #Get lorawan keys
        dev_uid_int = int(dev_uid, 16)
        deveui_str = dev_config.setdefault("deveui", hex(int(hashlib.sha256(dev_uid.encode('utf-8')).hexdigest()[:16],16)))
        deveui = int(deveui_str,16)
        appeui_str = dev_config.setdefault("appeui", global_appeui)
        appeui = int(appeui_str,16)
        appkey_str = dev_config.setdefault("appkey", hex(int(uuid.uuid4().hex,16)))
        appkey = int(appkey_str,16).to_bytes(16,byteorder='big')

        #Some info about the device
        dev_config["flash_size"] = hex(self.__flash_size)
        dev_config["MCU_ID"] = hex(self.__MCU_ID)
        dev_config["MCU"] = self.__MCU_name
        dev_config["CPU"] = self.__CPU_name
        dev_config["optionbytes"] = str(self.__optionbytes)

        #Struct for the lorawan keys to be stored in the last page of the device
        # 16 bytes: App key
        # 8 Keys: dev EUI
        # 8 Keys: app EUI (taken from global section)
        struct_data = struct.pack("<16BQQ", *(appkey), deveui , appeui)
        self.write_struct_data(struct_data)

        #Write optionbytes
        self.program_optionbytes()

        #Same devices file
        with open(DEVICES_LIST_FILE, 'w') as configfile:
            config.write(configfile)



    def read_data(self, address, length):
        READ_DATA_CMD = PROGRAMMER + " -r8 "+hex(address)+" "+str(length)
        try:
            read_uid_output = subprocess.check_output(READ_DATA_CMD.split(" "),universal_newlines=True)
        except subprocess.CalledProcessError as e:
            print("ERROR1: \n"+e.output)
            sys.exit(-1)
        
        self.extract_MCU_data(read_uid_output)
        
        valueArray = bytearray()
        for read_address in range (address,address+length, 16):
            read_address_str = hex(read_address)[2:]
            for line in read_uid_output.split('\n'):
                if read_address_str in line.lower():
                    valueArr = line.split(" ")[2:]
                    valueArray += bytes.fromhex("".join(valueArr))
                    break
            else:
                print("ERROR2: \n"+read_uid_output)
                sys.exit(-2)

        
        return valueArray

    def write_page(self, address, data):

        assert( len(data) <= self.__flash_page_size)

        offset_in_page = address % self.__flash_page_size
        start_page = address - offset_in_page
        page_index = (start_page - self.__flash_base) // self.__flash_page_size

        #First read page we are going to modify
        old_page_data = self.read_data(start_page, self.__flash_page_size)
        old_data = old_page_data[offset_in_page : offset_in_page+len(data)]
        if (data == old_data):
            print("data at "+hex(address)+" unchanged -> skipping write")
            return
        else:
            print("OLD DATA: "+old_data.hex())
            print("NEW DATA: "+data.hex())

        new_page_data = old_page_data[0:offset_in_page] + data + old_page_data[offset_in_page+len(data):]

        fp = tempfile.NamedTemporaryFile(delete=False, suffix=".bin")
        try:
            with fp:
                fp.write(new_page_data)
                fp.flush()
                WRITE_DATA_CMD = PROGRAMMER + " -e " + str(page_index) + ' -w "'+fp.name+'" '+hex(start_page)
                print(WRITE_DATA_CMD, flush=True)
                
                try:
                    read_uid_output = subprocess.check_output(WRITE_DATA_CMD.split(" "),universal_newlines=True)
                except subprocess.CalledProcessError as e:
                    print("ERROR3: \n"+e.output)
                    sys.exit(-3)
        finally:
            os.unlink(fp.name)


    def write_eeprom(self, address, data):

        #First read page we are going to modify
        old_data = self.read_data(address, len(data))
        if (data == old_data):
            print("data at "+hex(address)+" unchanged -> skipping write")
            return
        else:
            print("OLD DATA: "+old_data.hex())
            print("NEW DATA: "+data.hex())

        with tempfile.NamedTemporaryFile(suffix=".bin") as fp:
            fp.write(data)
            fp.flush()
            WRITE_DATA_CMD = PROGRAMMER + " -w "+fp.name+" "+hex(address)
            print(WRITE_DATA_CMD, flush=True)
            
            try:
                read_uid_output = subprocess.check_output(WRITE_DATA_CMD.split(" "),universal_newlines=True)
            except subprocess.CalledProcessError as e:
                print("ERROR3: \n"+e.output)
                sys.exit(-3)

    def get_uid(self):
        return self.read_data(self.__uid_addr, 12).hex()

    def write_struct_data(self, data):
        if self.__eeprom_address != 0:
            #Use EEPROM
            self.write_eeprom(self.__eeprom_address, data)
        else:
            #Use last flash page
            struct_address = self.__flash_base + self.__flash_size - self.__flash_page_size
            self.write_page(struct_address, data)


    def extract_MCU_data(self, programmer_output):

        for line in programmer_output.split('\n'):
            if "Flash size" in line:
                if "64 KBytes" in line:
                    self.__flash_size = 64 * 1024
                elif "128 KBytes" in line:
                    self.__flash_size = 128 * 1024
                else:
                    raise NameError("Unrecognised flash size")
            if "Device ID" in line:
                self.__MCU_ID = int(line.split(":")[1].strip(),16)
            if "Device name" in line:
                self.__MCU_name = line.split(":")[1].strip()
            if "Device CPU" in line:
                self.__CPU_name = line.split(":")[1].strip()
    
    def program_optionbytes(self):
        if (len(self.__optionbytes.keys()) == 0):
            return #No option bytes to set

        #Read option bytes
        READ_OPTIONBYTES_CMD = PROGRAMMER + " --optionbytes displ"
        try:
            read_optionbytes_output = subprocess.check_output(READ_OPTIONBYTES_CMD.split(" "),universal_newlines=True)
        except subprocess.CalledProcessError as e:
            print("ERROR4: \n"+e.output)
            sys.exit(-1)
        
        write_optionbytes = ""
        for (option, value) in self.__optionbytes.items():
            for line in read_optionbytes_output.split('\n'):
                if option in line:
                    #Found option
                    if value.lower() in line.lower():
                        print ("Option byte "+option+ " already set to "+value+" -> skipping set")
                    else:
                        write_optionbytes += " --optionbytes "+option+"="+value
                    break
            else:
                print("Option "+option+" not found in: \n"+read_optionbytes_output)
                sys.exit(-1)#Read option bytes
        
        if (len(write_optionbytes) == 0):
            return #Nothing to change
        
        for i in range(2):
            WRITE_OPTIONBYTES_CMD = PROGRAMMER + write_optionbytes
            print(WRITE_OPTIONBYTES_CMD)
            try:
                read_optionbytes_output = subprocess.check_output(WRITE_OPTIONBYTES_CMD.split(" "),universal_newlines=True)
                break
            except subprocess.CalledProcessError as e:
                print("ERROR5: \n"+e.output)
        else:
            sys.exit(-1)
        
        
        

################################################


parser = argparse.ArgumentParser(description='Init stm32 lorawan device keys')
parser.add_argument('--name', dest='name', default="noname",
                   help='name to be added to '+DEVICES_LIST_FILE)
parser.add_argument('--uid_address', dest='uid_address', default=str(UID_ADDRESS_L4),
                   help='address in flash for UID')
parser.add_argument('--eeprom_address', dest='eeprom_address', default=str(0),
                   help='address in flash for UID')
parser.add_argument('--optionbytes', dest='optionbytes', default={},
                    action = type('', (argparse.Action, ), dict(__call__ = lambda a, p, n, v, o: getattr(n, a.dest).update(dict([v.split('=')])))),
                    help='Optionbytes to set')


args = parser.parse_args()

print("name: "+args.name)
print("uid_address: "+args.uid_address)
print("eeprom_address: "+args.eeprom_address)
print("optionbytes: "+str(args.optionbytes))

device = lorawan_device(args.name,
                        uid_addr = int(args.uid_address, 16),
                        eeprom_address = int(args.eeprom_address, 16),
                        optionbytes = args.optionbytes)
device.program()
print ("Device Lorawan keys loaded")