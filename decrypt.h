#ifndef _DECRYPT_H_
#define _DECRYPT_H_
#include <string.h>

//returns a pointer to a string containing the decrypted message
char* decrypt(char* encrypted_string);

//removes every 8th character of the string passed to it
//returns a pointer to the newly stripped string
//note: not currently being used in code
char* Remove8th(char* encrypted_string, int newlen);

//compares the values to the table and returns the corresponding int
//table assumed to have exactly 41 characters
int str2num(char* table, char string);

//takes the decimal form array and converts to string
//returns a pointer to a string with corresponding letters to numarray from table
char* num2str(char* table, int* numarray, int len);

//takes an integer array in base 10 and converts to base 41
//returns a pointer to an array with base 41 values
int Dec2Base41(int* array, int pos, unsigned long long decimal);

//Modular Exponentiation using Right-to-Left Binary method
//citation in txt file
unsigned long long modularexp(unsigned long long base);

#endif
