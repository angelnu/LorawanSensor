#!/usr/bin/env python3
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
FLASH_SIZE_L4=0x20000

DEVICES_LIST_FILE="config/devices.ini" #put into a subfolder so it can be put into a different git repo
PROGRAMMER="STM32_Programmer_CLI -c port=swd freq=480"
DEFAULT_FLASH_BASE=0x08000000
DEFAULT_FLASH_PAGE_SIZE=0x800


class lorawan_device:

    def __init__(self,
                 name,
                 uid_addr,
                 flash_size,
                 flash_base=DEFAULT_FLASH_BASE,
                 flash_page_size=DEFAULT_FLASH_PAGE_SIZE):
        super().__init__()
        self.__name = name
        self.__uid_addr = uid_addr
        self.__flash_size = flash_size
        self.__flash_base = flash_base
        self.__flash_page_size = flash_page_size
    
    def program(self):

        config = configparser.ConfigParser(comment_prefixes='/', allow_no_value=True)
        os.makedirs(os.path.dirname(DEVICES_LIST_FILE), exist_ok=True)
        config.read(DEVICES_LIST_FILE)

        dev_uid = self.get_uid()
        print ("Detected device with UID "+dev_uid)

        if dev_uid not in config:
            print("Adding new device")
            config[dev_uid] = {}
        dev_config = config[dev_uid]

        if "global" not in config:
            config["global"] = {}
        
        global_appeui = config["global"].setdefault("appeui", hex(int(uuid.uuid4().hex,16) & 0xFFFFFFFFFFFFFFFF))

        dev_uid_int = int(dev_uid, 16)
        appeui = dev_config["name"] = self.__name
        deveui_str = dev_config.setdefault("deveui", hex(int(hashlib.sha256(dev_uid.encode('utf-8')).hexdigest()[:16],16)))
        deveui = int(deveui_str,16)
        appeui_str = dev_config.setdefault("appeui", global_appeui)
        appeui = int(appeui_str,16)
        appkey_str = dev_config.setdefault("appkey", hex(int(uuid.uuid4().hex,16)))
        appkey = int(appkey_str,16).to_bytes(16,byteorder='big')

        struct_data = struct.pack("<16BQQ", *(appkey), deveui , appeui)
        self.write_struct_data(struct_data)

        with open(DEVICES_LIST_FILE, 'w') as configfile:
            config.write(configfile)






    def read_data(self, address, length):
        READ_DATA_CMD = PROGRAMMER + " -r8 "+hex(address)+" "+str(length)
        try:
            read_uid_output = subprocess.check_output(READ_DATA_CMD.split(" "),universal_newlines=True)
        except subprocess.CalledProcessError as e:
            print("ERROR1: \n"+e.output)
            sys.exit(-1)
        
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

        with tempfile.NamedTemporaryFile(suffix=".bin") as fp:
            fp.write(new_page_data)
            fp.flush()
            WRITE_DATA_CMD = PROGRAMMER + " -e " + str(page_index) + " -w "+fp.name+" "+hex(start_page)
            print(WRITE_DATA_CMD, flush=True)
            
            try:
                read_uid_output = subprocess.check_output(WRITE_DATA_CMD.split(" "),universal_newlines=True)
            except subprocess.CalledProcessError as e:
                print("ERROR3: \n"+e.output)
                sys.exit(-3)

    def get_uid(self):
        return self.read_data(self.__uid_addr, 12).hex()

    def write_struct_data(self, data):
        struct_address = self.__flash_base + self.__flash_size - self.__flash_page_size
        self.write_page(struct_address, data)

################################################


parser = argparse.ArgumentParser(description='Init stm32 lorawan device')
parser.add_argument('--name', dest='name', default="noname",
                   help='name to be added to '+DEVICES_LIST_FILE)
parser.add_argument('--uid_address', dest='uid_address', default=UID_ADDRESS_L4,
                   help='address in flash for UID')
parser.add_argument('--flash_size', dest='flash_size', default=FLASH_SIZE_L4,
                   help='Flash memory in bytes')

args = parser.parse_args()

print("uid_address: "+args.uid_address)
print("flash_size: "+args.flash_size)

device = lorawan_device(args.name, int(args.uid_address, 16), int(args.flash_size, 16))
device.program()