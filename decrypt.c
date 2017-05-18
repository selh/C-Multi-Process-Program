#include <stdio.h>
#include <stdlib.h>
#include "decrypt.h"
#include "memwatch.h"
#include <math.h>

char* decrypt(char* encrypted_string) {
//printf("%s\n", encrypted_string );
   /*//STEP 1 remove every eight character from the string*/
  int len = strlen(encrypted_string)-2;

  int divsionby8 = floor(len/8); //check how many characters need to be removed
  int newlen = len - divsionby8;
  if(newlen%6 != 0){
    newlen++;
    len++;
  } //last line, since fgets reads until \n, it counts extra 2 except for last line

//printf("new len%d\n len %d\n", newlen, len );

  char* newstr = (char*)malloc(sizeof(char)*newlen); //allocate enough memory for temp string
  int count = 0, i=0, j=0, pos=1; //for loop variables

  for(i=0; i < len; i++){

   if( pos%8 != 0 || pos == 0){ // use seven since string starts at 0
     newstr[count++] = encrypted_string[i]; // copy everything except 8th letters
      //printf("encrypted_string%c\n", encrypted_string[i] );
   }
   pos++; //since iteration variable starts at 0 use pos instead
  }

  /* //STEP 2 number of remaining characters should be in groups of six
            including the spaces and punctutation. transform into base 41
  */

  count = 5; // count down to 0 ( to group into 6 characters )
  pos=0; // tracks the position in storecypher when adding new cyphers
  int value=0; //value holds the translated character value
  int numcyphers = newlen/6; //calculate how many cyphers there will be
  unsigned long long cypher=0 , M=0, tmp=0;
  int* numericalform = (int*)malloc(sizeof(int)*newlen); //holds the decyphered text
  unsigned long long* storecypher = (unsigned long long*)malloc(sizeof(unsigned long long)*numcyphers);
  char table[41] = {' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h','i', 'j', 'k', 'l', 'm', 'n', 'o', 'p','q','r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z','#','.', ',', '\'', '!', '?', '(', ')', '-', ':', '$', '/', '&','\\'};

  for(i=0; i< newlen; i++){

    for(j=0; j <= 40; j++){ //iterate through table values
      if( newstr[i] == table[j]){ //find matching character in table
          value = j;
          break;
          //printf("value in table is %c\n numerical value is %d\n", table[j], j );
      }
    }

    cypher = cypher + value*pow(41, count); //calculate value of cypher
    count--; //decrement the value until zero 20 · 41^5.. conversion to base 41

    if(count == -1 ){

      storecypher[pos++] = cypher; //place into array
      cypher = 0; // reset to 0
      count = 5;
    }

  }//end of for loop


  /*//STEP 3: Convert Cyphers into M and translate into base 41 to get final string
  */

  pos=0;//initialize variable
  for(i=0; i < numcyphers; i++){ //depending on the number of cyphers decrypt

    M = modularexp(storecypher[i]);
    //printf("value of M is %llu\n", M );
    Dec2Base41(numericalform, pos, M);
    pos= pos+6;
    //printf("value of pos is %d\n", pos );
  }

  char* finalstr = num2str(table, numericalform, newlen);

  //deallocate memory
  free(storecypher);
  free(newstr);
  free(numericalform);
  return finalstr;
}


//Modular Exponentiation
unsigned long long modularexp(unsigned long long base){

  unsigned long long power = 1921821779;
  unsigned long long n = 4294434817;
  unsigned long long remain = 1;
  int i =30;
  int binary[] = {1,1,1,0,0,1,0,1,0,0,0,1,1,0,0,1,0,1,0,1,1,0,0,0,1,0,1,0,0,1,1};

  ///second method
  unsigned long long res=1;
  base = base % n;
  while( i > -1 ){
    if( binary[i]%2 == 1){
      res = (res*base)%n;
    }
    i--;
    base= (base*base)%n;
  }

  return res;
}


//https://en.wikipedia.org/wiki/Modular_exponentiation
//algorithm used from wikipedia

//compares the string characters to the table and returns the corresponding integer
//only does 1 character at a time
int str2num(char* table, char string){
  int value;
  int j=0;
  for(j=0; j <= 40; j++){ //iterate through table values
    if( string == table[j]){ //find matching character in table
        value = j;
        //printf("value in table is %c\n numerical value is %d\n", table[j], j );
        break;
    }
  }
  return value;
}

//takes the decimal form array and converts to string
//returns a pointer to a string
//converts an entire array of ints into a string array
char* num2str(char* table, int* numarray, int len){

  char* strarray = (char*)malloc(sizeof(char)*(len+2));
  int j=0, i=0;

  memset(strarray, 0, len+2);
  for(j=0; j < len; j++){ //iterate through table values
     strarray[j] = table[numarray[j]];
  }

  return strarray;
}

//takes an integer array in base 10 and converts to base 41
//returns a pointer to an array with base 41 values
int Dec2Base41(int* array, int pos, unsigned long long decimal){

  int i, j;
  unsigned long long base41[6] = {0};

  for (i = 5; i > -1; i--){      //convert to base 41
    base41[i] = decimal%41;      //copy in backwards
    decimal = floor(decimal/41);
  }

  for(j=0; j < 6; j++){
    array[pos++] = base41[j];   //fill array from next blank pos
  }

  return 0;
}


/*
char* Remove8th(char* encrypted_string, int newlen){

  char* newstr = (char*)malloc(sizeof(char)*newlen); //allocate enough memory for temp string
  int count = 0;
  int i=0;

  for(i=0; i < len; i++){

       if( i%7 != 0 || i == 0){ // use seven since string starts at 0
         newstr[count++] = encrypted_string[i]; // copy everything except 8th letters
       }
  }
  printf("%s\n", newstr);
  return newstr;

}

*/
