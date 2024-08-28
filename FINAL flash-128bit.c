
#include<stdio.h>
#include "driverlib.h"
#include "device.h"
#include "FlashTech_F28P55x_C28x.h"
#include "flash_programming_f28p55x.h"

#define  WORDS_IN_FLASH_BUFFER    0x400


#pragma  DATA_SECTION(Buffer,"DataBufferSection");
uint16   Buffer[WORDS_IN_FLASH_BUFFER];
uint32   *Buffer32 = (uint32 *)Buffer;


void Example_Error(Fapi_StatusType status);
void Example_Done(void);
void Example_CallFlashAPI(void);
void FMSTAT_Fail(void);
void ECC_Fail(void);
void Example_EraseSector(void);


void Example_ProgramUsingAutoECC(void);
void ClearFSMStatus(void);



void Example_ReadFlash(uint32_t startAddress, uint32_t length);

void main(void)
{
    //
    // Initialize device clock and peripherals
    // Copy the Flash initialization code from Flash to RAM
    // Copy the Flash API from Flash to RAM
    // Configure Flash wait-states, fall back power mode, performance features
    // and ECC
    //
    Device_init();

    //
    // Initialize GPIO
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    //
    EINT;
    ERTM;

    //
    // At 150MHz, execution wait-states for external oscillator is 3. Modify the
    // wait-states when the system clock frequency is changed.
    //
    Flash_initModule(FLASH0CTRL_BASE, FLASH0ECC_BASE, 3);

    //
    // Flash API functions should not be executed from the same bank on which
    // erase/program operations are in progress.
    // Also, note that there should not be any access to the Flash bank on
    // which erase/program operations are in progress.  Hence below function
    // is mapped to RAM for execution.
    //
    Example_CallFlashAPI();

    //
    // Example is done here
    //

    puts("done");
    Example_Done();
}



//***********************************************************************************************
//  ClearFSMStatus
//
//  This function clears the status (STATCMD, similar to FMSTAT of the previous
//  devices) of the previous flash operation.
//  This function and the flash API functions used in this function are
//  executed from RAM in this example.
//  Note: this function is applicable for only F280013X, F280015X F28P55X and F28P65X devices
//***********************************************************************************************
#ifdef __cplusplus
#pragma CODE_SECTION(".TI.ramfunc");
#else
#pragma CODE_SECTION(ClearFSMStatus, ".TI.ramfunc");
#endif
void ClearFSMStatus(void){
    Fapi_FlashStatusType  oFlashStatus;
    Fapi_StatusType  oReturnCheck;

    //
    // Wait until FSM is done with the previous flash operation
    //
    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}
        oFlashStatus = Fapi_getFsmStatus();
        if(oFlashStatus != 0)
        {

            /* Clear the Status register */
            oReturnCheck = Fapi_issueAsyncCommand(Fapi_ClearStatus);
            //
            // Wait until status is cleared
            //
            while (Fapi_getFsmStatus() != 0) {}

            if(oReturnCheck != Fapi_Status_Success)
            {
                //
                // Check Flash API documentation for possible errors
                //
                Example_Error(oReturnCheck);
            }
        }
}



//*****************************************************************************
//  Example_CallFlashAPI
//
//  This function will interface to the flash API.
//  Flash API functions used in this function are executed from RAM in this
//  example.
//*****************************************************************************
#ifdef __cplusplus
#pragma CODE_SECTION(".TI.ramfunc");
#else
#pragma CODE_SECTION(Example_CallFlashAPI, ".TI.ramfunc");
#endif
void Example_CallFlashAPI(void)
{
    uint16 i = 0;
    Fapi_StatusType  oReturnCheck;

    //
    // Initialize the Flash API by providing the Flash register base address
    // and operating frequency(in MHz).
    // This function is required to initialize the Flash API based on System
    // frequency before any other Flash API operation can be performed.
    // This function must also be called whenever System frequency or RWAIT is
    // changed.
    //
    oReturnCheck = Fapi_initializeAPI(FlashTech_CPU0_BASE_ADDRESS, 
                                      DEVICE_SYSCLK_FREQ/1000000U);

    if(oReturnCheck != Fapi_Status_Success)
    {
        //
        // Check Flash API documentation for possible errors
        //
        Example_Error(oReturnCheck);
    }

    //
    // Initialize the Flash banks and FMC for erase and program operations.
    // Fapi_setActiveFlashBank() function sets the Flash banks and FMC for
    // further Flash operations to be performed on the banks.
    //
    oReturnCheck = Fapi_setActiveFlashBank(Fapi_FlashBank0);

    if(oReturnCheck != Fapi_Status_Success)
    {
        //
        // Check Flash API documentation for possible errors
        //
        Example_Error(oReturnCheck);
    }

    Example_EraseSector();
    //
    // Erase bank before programming
    //
    //Example_EraseBanks();

    //
    // Fill a buffer with data to program into the flash.
    //
    for(i=0; i < WORDS_IN_FLASH_BUFFER; i++)
    {
        Buffer[i] = i;
    }

    //
    // Program the sector using AutoECC option
    //
    Example_ProgramUsingAutoECC();



     Example_ReadFlash( FlashBank4StartAddress , 0x400U);
    //
    // Erase bank before programming
    //
    //Example_EraseBanks();

    //
    // Program the sector using DataOnly and ECCOnly options
    //
   // Example_ProgramUsingDataOnlyECCOnly();

    //
    // Erase the sector before programming
    //


    //
    // Program Bank using AutoECC option
    //
    // Note that executing entire bank needs a special flash API library -
    // for this
  //  Example_ProgramBankUsingAutoECC();

    //
    // Erase bank before programming
    //
   // Example_EraseBanks();

    //
    // Program the sector using DataAndECC option
    //
   // Example_ProgramUsingDataAndECC();

    //
    // Erase the sector for cleaner exit from the example.
    //
    //Example_EraseSector();


}

//*****************************************************************************
//  Example_ProgramUsingAutoECC
//
//  Example function to Program data in Flash using "AutoEccGeneration" option.
//  Flash API functions used in this function are executed from RAM in this
//  example.
//*****************************************************************************
#ifdef __cplusplus
#pragma CODE_SECTION(".TI.ramfunc");
#else
#pragma CODE_SECTION(Example_ProgramUsingAutoECC, ".TI.ramfunc");
#endif
void Example_ProgramUsingAutoECC(void)
{
    uint32 u32Index = 0;
    uint16 i = 0;
    Fapi_StatusType  oReturnCheck;
    Fapi_FlashStatusType  oFlashStatus;
    Fapi_FlashStatusWordType  oFlashStatusWord;

    //
    // A data buffer of max 8 16-bit words can be supplied to the program
    // function.
    // Each word is programmed until the whole buffer is programmed or a
    // problem is found. However to program a buffer that has more than 8
    // words, program function can be called in a loop to program 8 words for
    // each loop iteration until the whole buffer is programmed.
    //
    // Remember that the main array flash programming must be aligned to
    // 64-bit address boundaries and each 64 bit word may only be programmed
    // once per write/erase cycle.  Meaning the length of the data buffer
    // (3rd parameter for Fapi_issueProgrammingCommand() function) passed
    // to the program function can only be either 4 or 8.
    //
    // Program data in Flash using "AutoEccGeneration" option.
    // When AutoEccGeneration option is used, Flash API calculates ECC for the
    // given 64-bit data and programs it along with the 64-bit main array data.
    // Note that any unprovided data with in a 64-bit data slice
    // will be assumed as 1s for calculating ECC and will be programmed.
    //
    // Note that data buffer (Buffer) is aligned on 64-bit boundary for verify
    // reasons.
    //
    // Monitor ECC address for the sector below while programming with
    // AutoEcc mode.
    //
    // In this example, the number of bytes specified in the flash buffer
    // are programmed in the flash sector below along with auto-generated
    // ECC.
    //

    for(i=0, u32Index = FlashBank4StartAddress;
       (u32Index < (FlashBank4StartAddress + WORDS_IN_FLASH_BUFFER));
       i+= 8, u32Index+= 8)
    {
        ClearFSMStatus();

        // Enable program/erase protection for select sectors where this example is
        // located
        // CMDWEPROTA is applicable for sectors 0-31
        // Bits 0-11 of CMDWEPROTB is applicable for sectors 32-127, each bit represents
        // a group of 8 sectors, e.g bit 0 represents sectors 32-39, bit 1 represents
        // sectors 40-47, etc
        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE+FLASH_O_CMDWEPROTA, 0xFFFFFF00);
        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE+FLASH_O_CMDWEPROTB, 0x00000003);


        oReturnCheck = Fapi_issueProgrammingCommand((uint32 *)u32Index,Buffer+i,
                                               8, 0, 0, Fapi_AutoEccGeneration);

        //
        // Wait until the Flash program operation is over
        //
        while(Fapi_checkFsmForReady() == Fapi_Status_FsmBusy);

        if(oReturnCheck != Fapi_Status_Success)
        {
            //
            // Check Flash API documentation for possible errors
            //
            Example_Error(oReturnCheck);
        }

        //
        // Read FMSTAT register contents to know the status of FSM after
        // program command to see if there are any program operation related
        // errors
        //
        oFlashStatus = Fapi_getFsmStatus();
        if(oFlashStatus != 3)
        {
            //
            //Check FMSTAT and debug accordingly
            //
            FMSTAT_Fail();
        }

        //
        // Verify the programmed values.  Check for any ECC errors.
        //
        oReturnCheck = Fapi_doVerify((uint32 *)u32Index,     
                                     4, (uint32 *)(uint32)(Buffer + i),
                                     &oFlashStatusWord);

        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE+FLASH_O_CMDWEPROTA, 0xFFFFFFFF);

        if(oReturnCheck != Fapi_Status_Success)
        {
            //
            // Check Flash API documentation for possible errors
            //
            Example_Error(oReturnCheck);
        }
    }
}



#ifdef __cplusplus
#pragma CODE_SECTION(".TI.ramfunc");
#else
#pragma CODE_SECTION(Example_EraseSector, ".TI.ramfunc");
#endif
void Example_EraseSector(void)
{
    Fapi_StatusType  oReturnCheck;
    Fapi_FlashStatusType  oFlashStatus;
    Fapi_FlashStatusWordType  oFlashStatusWord;
        ClearFSMStatus();

        // Enable program/erase protection for select sectors where this example is 
        // located
        // CMDWEPROTA is applicable for sectors 0-31
        // Bits 0-11 of CMDWEPROTB is applicable for sectors 32-127, each bit represents
        // a group of 8 sectors, e.g bit 0 represents sectors 32-39, bit 1 represents
        // sectors 40-47, etc      
        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE+FLASH_O_CMDWEPROTA, 0xFFFFFF00);
        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE+FLASH_O_CMDWEPROTB, 0x00000000);


    //
    // Erase the sector that is programmed in the above example
    // Erase Sector 0
    //
    oReturnCheck = Fapi_issueAsyncCommandWithAddress(Fapi_EraseSector,
                   (uint32 *)FlashBank4StartAddress);
    //
    // Wait until FSM is done with erase sector operation
    //
    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady){}

    if(oReturnCheck != Fapi_Status_Success)
    {
        //
        // Check Flash API documentation for possible errors
        //
        Example_Error(oReturnCheck);
    }

    //
    // Read FMSTAT register contents to know the status of FSM after
    // erase command to see if there are any erase operation related errors
    //
    oFlashStatus = Fapi_getFsmStatus();
        if(oFlashStatus != 3)
    {
        //
        // Check Flash API documentation for FMSTAT and debug accordingly
        // Fapi_getFsmStatus() function gives the FMSTAT register contents.
        // Check to see if any of the EV bit, ESUSP bit, CSTAT bit or
        // VOLTSTAT bit is set (Refer to API documentation for more details).
        //
        FMSTAT_Fail();
    }

    //
    // Verify that Sector0 is erased
    //
    oReturnCheck = Fapi_doBlankCheck((uint32 *)FlashBank4StartAddress,
                   Sector2KB_u32length,
                   &oFlashStatusWord);
    if(oReturnCheck != Fapi_Status_Success)
    {
        //
        // Check Flash API documentation for error info
        //
        Example_Error(oReturnCheck);
    }
}



//******************************************************************************
// For this example, just stop here if an API error is found
//******************************************************************************
void Example_Error(Fapi_StatusType status)
{
    //
    //  Error code will be in the status parameter
    //
    __asm("    ESTOP0");
}

//******************************************************************************
//  For this example, once we are done just stop here
//******************************************************************************
void Example_Done(void)
{
    __asm("    ESTOP0");
}

//******************************************************************************
// For this example, just stop here if FMSTAT fail occurs
//******************************************************************************
void FMSTAT_Fail(void)
{
    __asm("    ESTOP0");
}

//******************************************************************************
// For this example, just stop here if ECC fail occurs
//******************************************************************************
void ECC_Fail(void)
{
    __asm("    ESTOP0");
}

//
// End of File
//
/*
void Example_ReadFlash(uint32_t startAddress, uint32_t length)
{
    uint16_t *flashPtr = (uint16_t *)startAddress;
    char buffer[64];
    uint32_t i;

    for (i = 0; i < length; i++)
    {
        // Read the content from flash memory
        uint16_t data = *(flashPtr + i);

        // Convert the data to a string and print it as an integer
        snprintf(buffer, sizeof(buffer), "Address: 0x%08X, Data: %u",
                 startAddress + (i * sizeof(uint16_t)), data);
        puts(buffer);
    }
}*/

void Example_ReadFlash(uint32_t startAddress, uint32_t length)
{
    uint16_t *flashPtr = (uint16_t *)startAddress;
    uint32_t i;

    for (i = 0; i < length; i++)
    {
        // Read the content from flash memory and print it as a raw hex value
        printf("%04X ", flashPtr[i]);

        // Read the content from flash memory and print it as a decimal value
         //printf("%5u ", flashPtr[i]);

        // Add a newline every 8 values for better readability
        if ((i + 1) % 8 == 0)
        {
            printf("\n");
        }
    }

    // Ensure we end with a newline
    if (length % 8 != 0)
    {
        printf("\n");
    }
}


