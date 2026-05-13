BOOTLOADER_TEST_LOG


# Test-1 (Bootloader and boot descriptor initialization)

- Date: 12/05/2026
- Objective: First boot initialization of the boot descriptor with no active firmware in either of the slots .
  
1. Erasing any previous firmware the bootloader is flashed in SECTOR - 0 of the flash memory (0x08000000 - 0x08003FFF)
2. Cube programmer is used with open ocd to debug and check each sector at register level.

- Serial_log 
     ![[Pasted image 20260512101805.png]]

# Test-2 (Application validation and jump for Slot-A)

- Date: 12/05/2026
- Objective: Application validation and jump from the bootloader 

1. The application is flashed via Cube programmer in Slot-A (112Kb) (0x08004000 - 0x0801FFFF )
2. The system is reset after and the directly loads the bootloader which validates the application and jumps to it.
3.  The main stack pointer and reset handler are pointed at the app address so that the app vector table can be read.

- Serial_log
     ![[Pasted image 20260512103653.png]]


# Test-3 (OTA Update from Slot-A to Slot-B)

- Date: 13/05/2026
- Objective: Application flashing and validation from an UART over the air update using a custom python script.

1. The python script sends the given app binary in 256 byte chunks and the mcu recieves it and compiles it to form the complete binary file of the application and flashes it to Slot-B (0x08040000 - 0x0805FFFF).
2. The the slot is validated and the bootloader jumps to Slot-B else it rolls back to Slot-A.
3. Time taken for one whole update cycle is 8 minutes. 

- Serial-log 
     ![[Pasted image 20260512114723.png|642]]  

# Test-4 (Slot-B persistence and no roll back)

- Date:12/05/2026
- Objective: To check if the board loads Slot-B application even after reset. 

1. The reset button is pressed and the mcu reverts to a previously loaded Slot-B over OTA.

- Serial_log
     ![[Pasted image 20260512121033.png]]

# Test-5 (Slot-B -> Slot-A OTA update / update with Slot-B active and flashing to Slot-A)

- Date:12/05/2026
- Objective: To flash the binary to Slot-A after the OTA was done on Slot-B and that slot is running the currently active firmware.

1. With Slot-B having the currently active and validated firmware the new firmware update is done through Slot-A to check if the bootloader can switch between the currently active slots.
2. The test was partially successful with ==UART log lagging to reach before the the app was jumped to reset. Hence that part of the code needs to be fixed.== 

- Serial_log 
     ![[Pasted image 20260512124028.png]]
     ![[Pasted image 20260512124155.png]]

# Test-6 (Bootloader validation CRC validation check through manual corruption of registers)

- Date:12/05/2026
- Objective: To check if after manual corruption of the binary files suing Cube Programmer the bootloader is able to detect fault and jump to a valid firmware or declare both firmware's as invalid.  

1. After corruption of both the slots the bootloader was able to check them and declare them as invalid.

- Serial_log 
     ![[Pasted image 20260512130740.png]]

2. After the corruption of Slot-A the firmware mcu will rollback to Slot-B hence proving that the CRC validation is functioning.

- Serial_log
     ![[Pasted image 20260512141422.png]]
     ![[Pasted image 20260512141728.png]]


3. After the corruption of Slot-B firmware mcu will rollback to Slot-A hence proving that the CRC validation is functioning.

- Serial_log 
      ![[Pasted image 20260512141956.png]]
      ![[Pasted image 20260512142059.png]]

# Test-7 (Invalid MSP checking and rejection)

- Date:12/05/2026
- Objective: To check whether if the bootloader rolls back to a working application after corruption of the MSP of applications in Slot-A or Slot-B.

1. After the corruption of MSP in slot-A the bootloader rolls to the Slot-B application. In the Cube programmer image one can see the MSP doesn't have the starting address of the SRAM (0x20020000).

- Serial_log 
     ![[Pasted image 20260512145035.png]]
     ![[Pasted image 20260512150128.png]]

2.  Doing the same but for  Slot-B's  application.

- Serial_log 
     ![[Pasted image 20260512151303.png]]
     ![[Pasted image 20260512151355.png]]

# Test-8 (Interrupted OTA recovery/ Power Loss during OTA)

- Date:12/05/2026
- Objective: To check whether the app is able to recover or roll back to the previous working firmware if the OTA is interrupted

1. The OTA right now can be interrupted if I just pull the USB cable from the laptop and also by stopping the python script while running.

- Serial_log 
     ![[Pasted image 20260512174621.png]]
     ![[Pasted image 20260512174700.png]]

# Test-9 (OTA flashed with corrupted chunks)

- Date:12/05/2026
- Objective: To check if the app validates corrupted firmware or not.

1. A custom python script for OTA that can generate corrupted blocks of firmware is flashed.

-  Serial_log
     ![[Pasted image 20260513092550.png]]

# Test-10 (Memory footprint/utilization analysis)

- Date:13/05/2026
- Objective: To check and document the memory utilization of the bootloader and both the test application and the memory in the SRAM.

1. We use the console logs from STM32 Cube IDE and also the the memory utilization console to check the amount of flash memory allocated to the bootloader ,  applications and check how much is utilized by each.

| Region                    | Used   | Available | %Used |
| ------------------------- | ------ | --------- | ----- |
| Bootloader(16Kb)          | 10.2Kb | 16Kb      | 64%   |
| Application_Slot-A(112Kb) | 8.8Kb  | 112Kb     | 7.9%  |
| Application_Slot-B(112Kb) | 8.8Kb  | 128Kb     | 6.9%  |
| RAM(128Kb)                | 2.1Kb  | 128Kb     | 1.6%  |


