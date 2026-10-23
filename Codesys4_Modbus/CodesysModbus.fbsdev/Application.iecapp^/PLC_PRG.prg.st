PROGRAM PLC_PRG
VAR
    counter : int ;
END_VAR

counter := counter + 1;

 modbus.valueFromCodesys :=  modbus.valueFromCodesys + 2;

 if modbus.valueFromCodesys > 100 then
      modbus.valueFromCodesys := 0;
 END_IF

END_PROGRAM
