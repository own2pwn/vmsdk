@REM This batch file has been generated by the IAR Embedded Workbench
@REM C-SPY Debugger, as an aid to preparing a command line for running
@REM the cspybat command line utility using the appropriate settings.
@REM
@REM Note that this file is generated every time a new debug session
@REM is initialized, so you may want to move or rename the file before
@REM making changes.
@REM
@REM You can launch cspybat by typing the name of this batch file followed
@REM by the name of the debug file (usually an ELF/DWARF or UBROF file).
@REM
@REM Read about available command line parameters in the C-SPY Debugging
@REM Guide. Hints about additional command line parameters that may be
@REM useful in specific cases:
@REM   --download_only   Downloads a code image without starting a debug
@REM                     session afterwards.
@REM   --silent          Omits the sign-on message.
@REM   --timeout         Limits the maximum allowed execution time.
@REM 


"C:\Program Files (x86)\IAR Systems\Embedded Workbench 6.5\common\bin\cspybat" "C:\Program Files (x86)\IAR Systems\Embedded Workbench 6.5\8051\bin\8051proc.dll" "C:\Program Files (x86)\IAR Systems\Embedded Workbench 6.5\8051\bin\8051sim.dll"  %1 --plugin "C:\Program Files (x86)\IAR Systems\Embedded Workbench 6.5\8051\bin\8051bat.dll" --backend -B "--proc_core" "plain" "--proc_code_model" "near" "--proc_nr_virtual_regs" "8" "--proc_pdata_bank_reg_addr" "0xA0" "--proc_dptr_nr_of" "1" "--proc_data_model" "large" "-p" "C:\Program Files (x86)\IAR Systems\Embedded Workbench 6.5\8051\config\devices\_generic\io8052.ddf" "--proc_driver" "sim" 


