#include <stdio.h>
#include <stdlib.h>	//Memory Management
#include <ctype.h> 	//For toupper()



// For readable reference. Currently implemented character set.
// Will be adding Punctuation and Prosigns.
char charMorse[] = { 	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
			'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
			'U', 'V', 'W', 'X', 'Y', 'Z', 
			'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};

//Encoded Morse Code: Follows the International ITU standard.
//Highest order 3 bits are the length of the following 5-bits.
//Lower order 5 bits are a binary encoding of morse code. (0-dot, 1-dash)
//Eg: 'A' = Morse '.-' = 01   Encoded as [2,0b01]=0b[010 00001]
char binaryMorse[] = {	0b01000001, 0b10001000, 0b10001010, 0b01100100, 0b00100000, //ABCDE
			0b10000010, 0b01100110, 0b10000000, 0b01000000, 0b10000111, //FGHIJ
			0b01100101, 0b10000100, 0b01000011, 0b01000010, 0b01100111, //KLMNO
			0b10000110, 0b10001101, 0b01100010, 0b01100000, 0b00100001, //PQRST
			0b01100001, 0b10000001, 0b01100011, 0b10001001, 0b10001011, //UVWXY
			0b10001100, 						    //Z
			0b10101111, 0b10100111, 0b10100011, 0b10100001, 0b10100000, //0-4 
			0b10110000, 0b10111000, 0b10111100, 0b10111110, 0b10111111};//5-9


//Gets the mapping index of the input character to the binary array of bits.
//Mapped {A-Z,0-9} (36 elements total)
int getIndex(char engChar) {
	char parsedChar = toupper(engChar);
	if (parsedChar >= 'A' && parsedChar <= 'Z')
		return parsedChar-'A'; //A-Z -> 0-25
	else if (parsedChar >= '0' && parsedChar <= '9')
		return 26+parsedChar-'0'; //0-9 -> 26-35
	
	return -1; //Sentinel value. Catches all odd characters. (\0, unmapped, etc).
}



//Prints a truncated byte, in binary. Data must be right-aligned.
void truncatedBinaryPrint(char printChar, int length) {
	for (int i = length-1; i >= 0; i--)
		printf("%d",(printChar >> i) & 1);
	printf("\n");
}
//Prints a full byte, in binary. Inserts a newline after each print.
void binaryPrint(char printChar) {
	for (int i = 7; i >= 0; i--)
		printf("%d",(printChar >> i) & 1);
	printf("\n");
}
//Prints a long charstring as binary. 
void printLongBinary(char* printString, int bitLength) {
	int byteIndex = 0;
	while (byteIndex < bitLength/8-1) {
		binaryPrint(printString[byteIndex]);
		byteIndex++;
	}
	if (bitLength%8 != 0 ) //As above catches everything but the last line.
		truncatedBinaryPrint( printString[byteIndex] >> (8-bitLength%8), bitLength%8);
}

//Print individual Morse Char in '01'->'.-' encoding.
void printMorseChar(char printChar) {
	char morseByte = binaryMorse[ getIndex(printChar) ]; //Fetch encoded value.
	char length = (morseByte >> 5) & 0b111; //Get upper order 3 bits.
	morseByte &= (1 << length)-1;
	truncatedBinaryPrint(morseByte,length);
	printf("\n");
}





//Seperated from getMorseString for clairity.
// Adds the bitstring to the array.
void addBits(char* targetString, int offset, char byteString, int length){
	for (int i = length-1; i >=0 ; i--) {	
		targetString[offset/8] |= (((byteString >> i) & 1) << (7-offset%8));
		offset++;
	}
}

// Translates a given byte to the proper encoding, pushes it to appends it to targetString.
// Returns the new offset
int addMorseChar(char* targetString, int offset, char letter) {
	//Edge cases: ' ', '!'.
	if (letter == ' ') { //Between words: 7 units of off. -3 as each letter appended with 0b000
		addBits(targetString, offset ,0x0,4);
		return offset + 4;
	}
	if (letter == '!') { //Between repeating lines: 36 units off
		addBits(targetString, offset    ,0x00,8);
		addBits(targetString, offset+ 8 ,0x00,8);
		addBits(targetString, offset+16 ,0x00,8);
		addBits(targetString, offset+24 ,0x00,8);
		addBits(targetString, offset+24, 0x00,8-offset%8);
		return offset + 32 + 8-offset%8; //Step up to nearest byte
	}

	char morseByte = binaryMorse[ getIndex(letter) ]; //Fetch encoded value.
	char length = (morseByte >> 5) & 0b111; //Returns upper order 3 bits.
	morseByte &= (1 << length)-1; //MorseByte is now morse-binary encoded.

	for (int  i = length-1; i >= 0; i--) {
		if (((morseByte >> i)&1) == 0) { //If Dot: 10
			addBits(targetString,offset,0b10,2);	
			offset += 2;
		}
		else { // If Dash: 1110
			addBits(targetString,offset,0b1110,4);
			offset += 4;
		}
	}
	addBits(targetString,offset,0b00,2); //3 Between each letter, -1 as each letter appended with 0.
	offset += 2;
	return offset;
}


// Primary Interface.
// Returns a char array following the standard Morse Broadcasting standard.
// 1) Dot length is 1 unit.
// 2) Dash length is 3 units.
// 3) Space between parts of the same letter is 1 unit.
// 4) Space between letters is 3 units
// 5) Space between words is 7 units.
// Translates an english string to a string for use in simple broadcasting.
//  Example is hooking up A UART to a Piezo Buzzer or something simple.
// Append '!' symbol for signals meant for repeat. Appends 36 off-bits, then rounds up to
//  nearest byte. (Temporary until Punctuation and Prosigns are added)

// On signal is 1, off signal is zero.
// Eg: "SOS" -> ...---... ->  1 1 1 111 111 111 1 1 1 -> 
//  ->  1 0 1 0 1 0 00 111 0 111 0 111 0 00 1 0 1 0 1 0 00
//  ->  10101000 11101110 11100010 101000
// And returns [4][6][Data]*

// System also adds two bytes to the beginning of the returned array,
//  Byte 1 is the byte-length of the returned string, excluding the data bytes. (Inc. partial byte)
//  Byte 2 is the bit-length of the final byte. 1-8

char* getMorseString(char* engString, int length) {
	//Inherent max length of the string is 255 bytes. (Max of char).
	char* returnString = malloc(255+2); //Just assume the worst. TODO: Upgrade.
	int bitLength = 16; //Keep running track of bitlength. 2 bytes for data.
	
	// for each char, translate and append to the returnString.	
	for (int i = 0; i < length && engString[i] != '\0'; i++)
		bitLength = addMorseChar(returnString, bitLength, engString[i]);		
		
	returnString[0] = bitLength/8 - 2; //Byte 1: Morse String length only.
	if (bitLength%8 != 0)
		returnString[0] = returnString[0] + 1; //Add 1, as divide rounds down.
	
	//Byte 2: Length of the final byte.
	if (bitLength%8 == 0)
		returnString[1] = 8;
	else
		returnString[1] = bitLength%8;
	
	return returnString;
}



int main(){
	char* morseStringArray;
	
	int offset = 0;

	morseStringArray = getMorseString("SOS",4);// Basic example. Command line args soon.
	int length = morseStringArray[0];
	int finalByte = morseStringArray[1]; 
	
	printf("Bytes: %d, final Byte Length: %d\n", length,finalByte);
	printf("Binary-Encoding begins:\n");

	printLongBinary(morseStringArray+2,8*length + finalByte);
	free (morseStringArray); //Because leaving malloc'd memory for removal at
				//program completion is a terrible habit.

	return 0;
}



