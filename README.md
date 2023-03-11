# iHex_tools
simple tool for creating IntelHEX file from binary file or with generated random data.

## IntelHEX file generator v1.0


Output file is generated input binary file or from random numbers

    iHex_tools.exe -o [-?] [-i] [-l] [-a] [-s] [-16bit]

    -?                    ... Show this help screen
    -h                    ... Show this help screen
    -o <output_file_name> ... Specifies output file name. This parameter cannot be ommited!
    -i <input_file_name>  ... Specified input file, which will be used for iHex file creation. File is taken as binary one!
    -l <file_length>      ... Output data size (in bytes). This parameter is incompatibile with -i parameter!
    -a <address>          ... Takes number (DEC/HEX) as start address of output data
    -s <seed>             ... Takes number as seed for random data generation. This parameter isn't used when -i parameter is used.
    -16bit                ... This flag switches between 32/16 bit file structure. (1MB address limit for 16b system. Other data types)
                              
                              