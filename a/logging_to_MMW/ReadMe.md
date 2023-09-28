# Sending Data to Monitor My Watershed/EnviroDIY <!-- {#example_mmw} -->

Port of ModularSensors to a SAMD51  prototype using a Wio Terminal to send to  https://monitormywatershed.org/    
The per site UUIDs are added in a ms_cfg.h file .    

The modem settings are for the onboard Wio Terminal WiFi.

The Wio Terminal (WioT) uses SAMD51/Cortex M4F processor with program flash of 512K Bytes and ram of 192K Bytes
(The Mayfly mega1284 has program flash 128KBytes and 16K Bytes)    

https://www.seeedstudio.com/Wio-Terminal-p-4509.html
The WioT has following feautures  
 LCD screen 2.4inches  
 WiFi/BlueTooth   
 2 useable UARTS out of 8 SERCOM devices   
 USB OTG Console port   
 2 Seeed connectors - ADC 12bits.
 microSD card Slot - max 16GB   
 onboard 4MB local flash chip   
 40 Pin connector, Raspberry Pi Format, 29useable data pins   
    

 Diary using Wio Terminal build   
 230216: Low Power testing IDLE2    
 SAMD51 clock can be set from 120MHz to 48Mhz at compile.   
 SAMD51 has various sleep modes - using IDLE2.    
 For lower power STANDBY the Serial1 doesn't recover   

 48Mhz using Serial1/IDEL2 disabling all onboard pins sleep=5mA wake=9mA - no WiFi on wake   
 48Mhz using Serial1/IDEL2 running MS overnight on a one munute sampling schedule sleep24mA wake30mA    

 48Mhz using Serial1/STANDBY disabling all onboard pins sleep3mA wake9mA - Serial1 doesn't recover   
 Testing on a Adafruit Express M4 - similar SAMD51 - had  sleep1.3mA and wake3.9mA    

 Sensors

 https://www.seeedstudio.com/DS18B20-Temperature-Sensor-Waterproof-Probe-p-4283.html
 

_______

[//]: # ( @tableofcontents )

[//]: # ( @m_footernavigation )

[//]: # ( Start GitHub Only )
- [Sending Data to Monitor My Watershed/EnviroDIY](#sending-data-to-monitor-my-watershedenvirodiy)
- [Unique Features of the Monitor My Watershed Example](#unique-features-of-the-monitor-my-watershed-example)
- [To Use this Example](#to-use-this-example)
  - [Prepare and set up PlatformIO](#prepare-and-set-up-platformio)
  - [Set the logger ID](#set-the-logger-id)
  - [Set the universally universal identifiers (UUID) for each variable](#set-the-universally-universal-identifiers-uuid-for-each-variable)
  - [Upload!](#upload)

[//]: # ( End GitHub Only )

_______

# Unique Features of the Monitor My Watershed Example <!-- {#example_mmw_unique} -->
- A single logger publishes data to the Monitor My Watershed data portal.
- Uses a cellular Digi XBee or XBee3

# To Use this Example <!-- {#example_mmw_using} -->

## Prepare and set up PlatformIO <!-- {#example_mmw_pio} -->
- Register a site and sensors at the Monitor My Watershed/EnviroDIY data portal (http://monitormywatershed.org/)
- Create a new PlatformIO project
- Replace the contents of the platformio.ini for your new project with the [platformio.ini](https://raw.githubusercontent.com/EnviroDIY/ModularSensors/master/examples/logging_to_MMW/platformio.ini) file in the examples/logging_to_MMW folder on GitHub.
    - It is important that your PlatformIO configuration has the lib_ldf_mode and build flags set as they are in the example.
    - Without this, the program won't compile.
- Open [logging_to_MMW.ino](https://raw.githubusercontent.com/EnviroDIY/ModularSensors/master/examples/logging_to_MMW/logging_to_MMW.ino) and save it to your computer.
    - After opening the link, you should be able to right click anywhere on the page and select "Save Page As".
    - Move it into the src directory of your project.
    - Delete main.cpp in that folder.

## Set the logger ID <!-- {#example_mmw_logger_id} -->
- Change the settings in ms_cfg.h <tbd>  :

```cpp
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char *LoggerID = "XXXX";
```

## Set the universally universal identifiers (UUID) for each variable <!-- {#example_mmw_uuids} -->
- Go back to the web page for your site at the Monitor My Watershed/EnviroDIY data portal (http://monitormywatershed.org/)
- For each variable, find the dummy UUID (`"12345678-abcd-1234-ef00-1234567890ab"`) and replace it with the real UUID for the variable.

## Upload! <!-- {#example_mmw_upload} -->
- Test everything at home **before** deploying out in the wild!

_______

[//]: # ( @section example_mmw_pio_config PlatformIO Configuration )

[//]: # ( @include{lineno} logging_to_MMW/platformio.ini )

[//]: # ( @section example_mmw_code The Complete Code )

[//]: # ( @include{lineno} logging_to_MMW/logging_to_MMW.ino )
