#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define MAX_DATA_PER_LINE   0x10
#define MAX_FILE_NAME_LEN   0xFF

typedef enum
{
    iHex_record_type_data              = 0,
    iHex_record_type_end_file          = 1,
    iHex_record_type_16b_address       = 2,
    iHex_record_type_16b_start_address = 3,
    iHex_record_type_32b_address       = 4,
    iHex_record_type_32b_start_address = 5
} e_iHex_recort_type;

typedef enum
{
    iHex_bit_length_type_16bit,
    iHex_bit_length_type_32bit
} e_iHex_bit_length_type;

uint32_t _iHex_generate_line(char* output_buffer, uint8_t data_count, uint16_t address, e_iHex_recort_type data_type, uint8_t* data_ptr)
{
    uint8_t checksum;
    uint32_t str_len;

    str_len = sprintf(output_buffer, ":%02X%04X%02X", data_count, address, (uint8_t)data_type);
    checksum = 0 - data_count - (address >> 8) - (address & 0xFF) - (uint8_t)data_type;
    for (uint8_t i = 0; i < data_count; i++)
    {
        str_len += sprintf(output_buffer + str_len, "%02X", *data_ptr);
        checksum -= *data_ptr;
        data_ptr++;
    }
    str_len += sprintf(output_buffer + str_len, "%02X\n", checksum);
    return str_len;
}

uint32_t _iHex_generate_file(const char *output_file_name, uint32_t *current_address, uint32_t end_address, uint8_t *data_ptr, uint32_t data_count, e_iHex_bit_length_type iHex_bit_length_type)
{
    FILE *output_file_stream = fopen(output_file_name, "a+");
    uint32_t line_address;
    static uint32_t address_offset = 0;
    uint32_t data_size = 0;
    uint32_t iHex_string_len = 0;
    size_t fw;
    char iHex_line[13 + MAX_DATA_PER_LINE * 2];

    if(output_file_stream == NULL)
    {
        return 4;
    }

    for(uint32_t data_position = 0; data_position < data_count; data_position += data_size)
    {
        _Bool address_line_generation = 0;

        if(data_position > data_count)
        {
            return 5;
        }
        
        if(iHex_bit_length_type == iHex_bit_length_type_16bit)
        {
            if(*current_address - (address_offset << 4) > 0xFFFF)
            {
                address_offset = *current_address >> 4;
                iHex_string_len = _iHex_generate_line(iHex_line, 2, 0, iHex_record_type_16b_address, (uint8_t[])
                {
                    address_offset >> 8, address_offset & 0xFFU
                });
                address_line_generation = 1;
                data_size = 0;
            }
        }
        else
        {
            if(address_offset != (*current_address >> 16))
            {
                address_offset = *current_address >> 16;
                iHex_string_len = _iHex_generate_line(iHex_line, 2, 0, iHex_record_type_32b_address, (uint8_t[])
                {
                    address_offset >> 8, address_offset & 0xFFU
                });
                address_line_generation = 1;
                data_size = 0;
            }
        }
        
        if(!address_line_generation)
        {
            if(iHex_bit_length_type == iHex_bit_length_type_16bit)
            {
                line_address = *current_address - (address_offset << 4);
            }
            else
            {
                line_address = *current_address - (address_offset << 16);
            }
            data_size = (end_address - *current_address);

            if(data_size > MAX_DATA_PER_LINE)
            {
                data_size = MAX_DATA_PER_LINE;
                if(0x10000UL - (line_address & 0x0000FFFFU) < MAX_DATA_PER_LINE)
                {
                    data_size = 0xFFFFUL - (line_address & 0xFFFFU);
                }
            }

            if(line_address % MAX_DATA_PER_LINE)
            {
                data_size = line_address & 0xFFFFU;
                data_size = MAX_DATA_PER_LINE - (line_address % MAX_DATA_PER_LINE);
            }

            //printf("%d params: %p %d %d %d %p\r\n", __LINE__, iHex_line, data_size, line_address, iHex_record_type_data, data_ptr + data_position);
            iHex_string_len = _iHex_generate_line(iHex_line, data_size, line_address, iHex_record_type_data, data_ptr + data_position);
            if(iHex_string_len > sizeof(iHex_line) / sizeof(*iHex_line))
            {
                fclose(output_file_stream);
                return 1;
            }
            *current_address += data_size;
        }

        if(iHex_string_len > sizeof(iHex_line))
        {
            fclose(output_file_stream);
            return 1;
        }

        if(iHex_string_len != 0)
        {
            //printf("%d params: %s %d %d %p\r\n", __LINE__, iHex_line, sizeof(*iHex_line), iHex_string_len, output_file_stream);
            fw = fwrite(iHex_line, sizeof(*iHex_line), iHex_string_len, output_file_stream);
            //printf("fw = %d\r\n", fw);
            if(fw != iHex_string_len)
            {
                fclose(output_file_stream);
                return 2;
            }
        }
        //printf("%d data_size=%d, actual=%d, total=%d\r\n", __LINE__, data_size, data_position, data_count);
    }

    if(*current_address >= end_address)
    {
        iHex_string_len = _iHex_generate_line(iHex_line, 0, 0, iHex_record_type_end_file, NULL);
        if(iHex_string_len > sizeof(iHex_line) / sizeof(*iHex_line))
        {
            fclose(output_file_stream);
            return 1;
        }
        fw = fwrite(iHex_line, sizeof(*iHex_line), iHex_string_len, output_file_stream);
        //printf("%d fw = %d\r\n", __LINE__, fw);
        if(fw != iHex_string_len)
        {
            fclose(output_file_stream);
            return 2;
        }
    }
    fclose(output_file_stream);
    return 0;
}

const char *help_text = "IntelHEX file generator v1.0\r\n\r\n" \
                        "  Output file is generated input binary file or from random numbers\r\n" \
                        "  Function syntax: iHex_tools.exe -o [-?] [-i] [-l] [-a] [-s] [-16bit]\r\n" \
                        "    -o <output_file_name> ... Specifies output file name.\r\n" \
                        "                              This parameter cannot be ommited!\r\n" \
                        "    -i <input_file_name>  ... Specified input file, which will be used for iHex\r\n" \
                        "                              file creation. File is taken as binary one!\r\n" \
                        "    -l <file_length>      ... Output data size (in bytes).\r\n" \
                        "                              This parameter is incompatibile with -i parameter!\r\n" \
                        "    -a <address>          ... Takes number (DEC/HEX) as start address of output data\r\n" \
                        "    -s <seed>             ... Takes number as seed for random data generation.\r\n" \
                        "                              This parameter isn't used when -i parameter is used.\r\n" \
                        "    -16bit                ... This flag switches between 32/16 bit file structure.\r\n" \
                        "                              (1MB address limit for 16b system. Other data types)\r\n";

int main(int argc, char **argv)
{
    FILE *input_file_stream;
    char output_file_name[MAX_FILE_NAME_LEN + 1] = {0};
    char input_file_name[MAX_FILE_NAME_LEN + 1] = {0};
    uint32_t output_data_len = 0;
    uint32_t start_address = 0;
    e_iHex_bit_length_type iHex_bit_length_type = iHex_bit_length_type_32bit;
    _Bool out_file_specified = 0;
    _Bool output_data_len_specified = 0;
    _Bool input_file_specified = 0;

    for(uint8_t i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-' || argv[i][0] == '/')
        {
            if(argv[i][1] == '?' || argv[i][1] == 'h')
            {
                printf(help_text);
                return 0;
            }

            if(argc < i + 2)
            {
                printf("Missing argument parameter for \"%s\"!\r\n", argv[i]);
                return 1;
            }

            if(strlen(argv[i + 1]) == 0)
            {
                printf("Unknown parameter value of %s!\r\n", argv[i]);
                return 1;
            }

            uint32_t temporary_string_len;
            switch(argv[i][1])
            {
                case 'o':
                    temporary_string_len = strlen(argv[i + 1]);
                    if(temporary_string_len > MAX_FILE_NAME_LEN)
                    {
                        temporary_string_len = MAX_FILE_NAME_LEN;
                    }
                    strncpy(output_file_name, argv[i + 1], temporary_string_len);
                    output_file_name[temporary_string_len] = '\0';
                    out_file_specified = 1;
                    break;
                case 'i':
                    temporary_string_len = strlen(argv[i + 1]);
                    if(temporary_string_len > MAX_FILE_NAME_LEN)
                    {
                        temporary_string_len = MAX_FILE_NAME_LEN;
                    }
                    strncpy(input_file_name, argv[i + 1], temporary_string_len);
                    output_file_name[temporary_string_len] = '\0';
                    input_file_specified = 1;
                    break;
                case 'l':
                    if(argv[i + 1][1] == 'x')
                    {
                        output_data_len = strtoul(argv[i + 1], NULL, 16);
                    }
                    else
                    {
                        output_data_len = strtoul(argv[i + 1], NULL, 10);
                    }
                    output_data_len_specified = 1;
                    break;
                case 'a':
                    if(argv[i + 1][1] == 'x')
                    {
                        start_address = strtoul(argv[i + 1], NULL, 16);
                    }
                    else
                    {
                        start_address = strtoul(argv[i + 1], NULL, 10);
                    }
                    break;
                case 's':
                    if(argv[i + 1][1] == 'x')
                    {
                        srand(strtoul(argv[i + 1], NULL, 16));
                    }
                    else
                    {
                        srand(strtoul(argv[i + 1], NULL, 10));
                    }
                    break;
                case '1':
                    if(strcmp(&argv[i][1], "16") == 0 || strcmp(&argv[i][1], "16bit") == 0)
                    {
                        iHex_bit_length_type = iHex_bit_length_type_16bit;
                    }
                    break;
                default:
                    printf("Unknown parameter %s!\r\n", argv[i]);
                    return 1;
            }
            i++;
        }
        else
        {
            printf("Error, unknown argument or parameter!\r\n");
            return 1;
        }
    }

    if(output_data_len_specified && input_file_specified)
    {
        printf("Unable to use output generation size parameter and input data file at once!\r\n");
        return 1;
    }

    if(input_file_specified)
    {
        input_file_stream = fopen(input_file_name, "rb");
        if(input_file_stream == NULL)
        {
            printf("Error, unknown input file!\r\n");
            return 1;
        }

        fseek(input_file_stream, 0, SEEK_END);
        output_data_len = ftell(input_file_stream);
        fclose(input_file_stream);
        output_data_len_specified = 1;
    }

    if(!out_file_specified)
    {
        printf("Output file must be specified!\r\n");
        return 1;
    }

    if(!output_data_len_specified)
    {
        printf("Data length or input file must be specified!\r\n");
        return 1;
    }

    if(iHex_bit_length_type == iHex_bit_length_type_16bit && (start_address + output_data_len > 0x100000))
    {
        printf("Invalid address space or data size for 16bit hex file!\r\n");
        return 2;
    }

    input_file_stream = fopen(output_file_name, "w"); // clean output file :)
    fclose(input_file_stream);

    if(input_file_specified)
    {
        input_file_stream = fopen(input_file_name, "rb");
        if(input_file_stream == NULL)
        {
            printf("Error, unknown input file!\r\n");
            return 1;
        }
    }

    uint8_t fbuffer[0x101];
    uint32_t end_address = start_address + output_data_len;
    uint32_t frcnt = 0;
    while(output_data_len > 0)
    {
        if(input_file_specified)
        {
            frcnt = fread(fbuffer, 1, 0x100, input_file_stream);
        }
        else
        {
            if(output_data_len > 0x100)
            {
                frcnt = 0x100;
            }
            else
            {
                frcnt = output_data_len;
            }
            for(uint32_t n = 0; n < frcnt; n++)
            {
                fbuffer[n] = (uint8_t)(rand() & 0xFFU);
            }
        }
        output_data_len -= frcnt;
        //printf("params:%s %d %d %p %d %d\r\n", output_file_name, start_address, end_address, fbuffer, frcnt, iHex_bit_length_type);
        uint32_t t = _iHex_generate_file(output_file_name, &start_address, end_address, fbuffer, frcnt, iHex_bit_length_type);
        if(t != 0)
        {
            printf("File generation problem ocured! retval = %d\r\n", t);
            return 3;
        }
        //printf("%d remaining file len=%d\r\n", __LINE__, output_data_len);
    }
    
    fclose(input_file_stream);
    return 0;
}
