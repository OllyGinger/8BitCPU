#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4
#define EEPROM_D0 5
#define EEPROM_D7 12
#define WRITE_EN 13

void setAddress(unsigned int address, bool outputEnable) 
{
    shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address >> 8) | (outputEnable ? 0x00 : 0x80));
    shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, address);

    digitalWrite(SHIFT_LATCH, LOW);
    digitalWrite(SHIFT_LATCH, HIGH);
    digitalWrite(SHIFT_LATCH, LOW);
}

byte readEEPROM(unsigned int address)
{
    for(int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1)
    {
        pinMode(pin, INPUT);
    }

    setAddress(address, true);
    byte data = 0;
    for(int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1)
    {
        data = (data << 1) + digitalRead(pin);
    }
    return data;
}

void writeEEPROM(unsigned int address, byte data)
{
    setAddress(address, false);
    for(int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1)
    {
        pinMode(pin, OUTPUT);
    }

    for(int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1)
    {
        digitalWrite(pin, data & 1);
        data >>= 1;
    }

    digitalWrite(WRITE_EN, LOW);
    delayMicroseconds(1);
    digitalWrite(WRITE_EN, HIGH);
    delay(10);
}

void printContents()
{
    for(int base = 0x400; base <= 2048; base += 16) 
    {
        byte data[16];
        for(int offset = 0; offset <= 15; ++offset)
        {
            data[offset] = readEEPROM(base + offset);
        }

        char buffer[80];
        snprintf(buffer, sizeof(buffer), "%03x:   %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

        Serial.println(buffer);
    }
}

void write8SegmentLUT()
{
    /* Handles 8 segment pins in the following order:
               7 6 x 5 4
                 ____ 
                |    |
                |    |
                |____|
               0 1 x 2 3
    */

   //                         0     1     2    3     4      5     6    7     8      9
   const byte kMapping[] = {0x77, 0x14, 0xb3, 0xb6, 0xd4, 0xe6, 0xc7, 0x34, 0xf7, 0xf4};

    // Program addresses to match single digit

    // Address bits 8, 9, 10 are used to select the digit
    //
    // 10  9  8  | 7  6  5  4  | 3  2  1  0
    // ----------+-------------+-----------
    // 0   0  0    0  0  0  0    0  0  0  0   | Least Significant Digit
    // 0   0  1    0  0  0  0    0  0  0  0   | 2nd Significant Digit
    // 0   1  0    0  0  0  0    0  0  0  0   | 3rd Significant Digit
    // 0   1  1    0  0  0  0    0  0  0  0   | Most Significant Digit

    // 1st Digit
    int startAddress = 0;
    for(int address = startAddress; address <= 255; ++address)
    {
        writeEEPROM(address, kMapping[address % 10]);
    }

    // 2nd Digit
    startAddress = 256; // 0b0001 0000 0000
    for(int address = 0; address <= 255; ++address)
    {
        writeEEPROM(address + startAddress, kMapping[(address / 10) % 10]);
    }

    // 3rd Digit
    startAddress = 512; // 0b0010 0000 0000
    for(int address = 0; address <= 255; ++address)
    {
        writeEEPROM(address + startAddress, kMapping[(address / 100) % 10]);
    }

    // 4th Digit
    startAddress = 768; // 0b0011 0000 0000
    for(int address = 0; address <= 255; ++address)
    {
        writeEEPROM(address + startAddress, 0);
    }


    // --- 2s complement digits ---
    Serial.println("Programming onece place (twos complement)");
    for (int value = -128; value <= 127; value += 1) 
    {
        writeEEPROM((byte)value + 1024, kMapping[abs(value) % 10]);
    }

    Serial.println("Programming tens place (twos complement)");
    for (int value = -128; value <= 127; value += 1) 
    {
        writeEEPROM((byte)value + 1280, kMapping[abs(value / 10) % 10]);
    }
    Serial.println("Programming hundreds place (twos complement)");
    for (int value = -128; value <= 127; value += 1) 
    {
        writeEEPROM((byte)value + 1536, kMapping[abs(value / 100) % 10]);
    }
    Serial.println("Programming sign (twos complement)");
    for (int value = -128; value <= 127; value += 1) 
    {
        if (value < 0) 
        {
            writeEEPROM((byte)value + 1792, 0x80);
        } 
        else 
        {
            writeEEPROM((byte)value + 1792, 0);
        }
    }
}

void setup()
{
	Serial.begin(115200);

    pinMode(SHIFT_DATA, OUTPUT);
    pinMode(SHIFT_CLK, OUTPUT);
    pinMode(SHIFT_LATCH, OUTPUT);

    digitalWrite(WRITE_EN, HIGH);
    pinMode(WRITE_EN, OUTPUT);

    /*Serial.print("Erasing EEPROM...");
    for(int addr = 0; addr < 2048; ++addr)
    {
        writeEEPROM(addr, 0x00);
        if(addr % 64 == 0)
        {
            Serial.print(".");
        }
    }
    Serial.println(" done.");*/

    write8SegmentLUT();
    printContents();
}

void loop()
{
	
}
