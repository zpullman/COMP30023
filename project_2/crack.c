#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include "sha256.h"
#include "crack.h"

int main(int argc, char *argv[]) {
  BYTE bword[PWLENGTH];
  char word[PWLENGTH];
  FILE *words;

  if(argc>3) {
    fprintf(stderr, "Only takes in 2 arguments max\n");
    fprintf(stderr, "No args prints out found passwords, 1 arg prints number of guesses\n");
    fprintf(stderr, "2 args \n");
    return 0;
  }
  if(argc==2) {
    int nguesses = atoi(argv[1]);
    if(nguesses <= 0) {
      fprintf(stderr, "input needs to be positive integer values\n");
      return 0;
    }
    return guessing(nguesses);
  }
/** Read in first the dictionary and then the hashes
  *
*/
  if(argc==3) {
    char * dictName = NULL;
    char * hashFName = NULL;

    dictName = argv[1];
    hashFName = argv[2];
    words = fopen(dictName, "r");

    while( fgets(word,PWLENGTH,words) != NULL ) {
      chop(word);
      int len = strlen(word);
      string2ByteArray(word,bword);
      checkpws(bword, hashFName, 1,len,word);
    }
  fclose(words);

  return 1;
  }
  // No args so run over passwords
  if(argc == 1) {
    words = fopen("passwords", "r"); /* open spelling dictionary */


    while( fgets(word,PWLENGTH,words) != NULL ) {
      chop(word);
      string2ByteArray(word,bword);
      checkpw(bword, "pwd4sha256", 1);
    }
    rewind(words);
    while( fgets(word,PWLENGTH,words) != NULL ) {
      chop(word);
      string2ByteArray(word,bword);
      checkpw(bword, "pwd6sha256", 11);
    }

    fclose(words);
    return 1;
  }
}

 /* chops a \n off the end of a word, replaces with \0 */
void chop(char *word) {
  int lenword;
  lenword=strlen(word);
  if(lenword>1) {
    if( word[lenword-1] == '\n')
      word[lenword-1] = '\0';
    }
}

// https://www.includehelp.com/c/convert-ascii-string-to-byte-array-in-c.aspx
//function to convert string to byte array
void string2ByteArray(char* input, BYTE* output)
{
    int loop = 0;
    int i = 0;
    while(input[loop] != '\0')
    {
        output[i++] = input[loop++];
    }
}

void checkpw(BYTE text[], char * fname, int index) {
  BYTE hashedguess[SHA256_BLOCK_SIZE];
  //char copy[PWLENGTH];
  //memcpy(copy, text, PWLENGTH*sizeof(char));
  //copy[strlen(copy)] = '\0';
  SHA256_CTX ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, text, strlen((char*)text));
  sha256_final(&ctx, hashedguess);
  BYTE pwhash[SHA256_BLOCK_SIZE];
  struct stat st;

  int i, fsize, numhashes;
  FILE *fp;
  stat(fname, &st);
  fsize = st.st_size;
  numhashes = fsize/SHA256_BLOCK_SIZE;
  fp = fopen(fname, "r");
  for(i = 0; i<numhashes; i++) {
    fread(pwhash,1,SHA256_BLOCK_SIZE,fp);
    if(memcmp(hashedguess,pwhash,SHA256_BLOCK_SIZE)==0) {
      printf("%s %i\n",(char*)text, i+index);
    }
  }
  fclose(fp);
}

void checkpws(BYTE text[], char * fname, int index, int len, char * word) {
  BYTE hashedguess[SHA256_BLOCK_SIZE];
  char copy[strlen((char*)text)];
  memcpy(copy, text, strlen((char*)text)*sizeof(char));
  copy[strlen(copy)-1] = '\0';
  SHA256_CTX ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, text, len);
  sha256_final(&ctx, hashedguess);
  BYTE pwhash[SHA256_BLOCK_SIZE];
  struct stat st;

  int i, fsize, numhashes;
  FILE *fp;
  stat(fname, &st);
  fsize = st.st_size;
  numhashes = fsize/SHA256_BLOCK_SIZE;
  fp = fopen(fname, "r");
  for(i = 0; i<numhashes; i++) {
    fread(pwhash,1,SHA256_BLOCK_SIZE,fp);
    if(memcmp(hashedguess,pwhash,SHA256_BLOCK_SIZE)==0) {
      //word[strlen(word)] = '\0';     //no idea why this works or why its necessary
      printf("%s %i\n",word, i+index); // something to do wuth memory but found out with 5 hours left
    }
  }
  fclose(fp);
}

void crack4chars(char*fileName) {
  char word[5];
  BYTE bword[5];
  int q, w, e,r;

  for(q=48; q<123;q++) {
    for(w=48; w<123;w++) {
      for(e=48; e<123;e++) {
        for(r=48; r<123;r++) {
          word[0] = q;
          word[1] = w;
          word[2] = e;
          word[3] = r;
          word[4] = '\0';
          string2ByteArray(word,bword);
          checkpw(bword, fileName, 1);
        }
      }
    }
  }
}

  void brute6chars(char* fileName) {
    char word[7];
    BYTE bword[7];
    int q, w, e,r,t,y;

    for(q=32; q < 123; q++) {
      for(w=32; w < 123; w++) {
        for(e=32; e < 123; e++) {
          for(r=32; r < 123; r++) {
            for(t=32; t < 123; t++) {
              for(y=32; y < 123; y++) {
                word[0] = q;
                word[1] = w;
                word[2] = e;
                word[3] = r;
                word[4] = t;
                word[5] = y;
                word[6] = '\0';
                string2ByteArray(word,bword);
                checkpw(bword, fileName, 1);
              }
            }
          }
        }
      }
    }
  }

/*
** The guessing function changes each char to capital, starting from the 1st
** char. Then iterates across, this is because most people have caps at the start
** of there passowrds.
** The second function just replaces the common passowrds with characters,
** i was unable to make it work the way i initially intended by putting in most
** common substitutions in first. this will hae to be good enough.
*/
  int guessing(int nguesses) {
    FILE * words;
    //words = fopen("proj-2_common_passwords.txt", "r"); /* open spelling dictionary */
    words = fopen("6letter_dict.txt", "r"); /* open spelling dictionary */
    char guess[PWLENGTH];
    int count = 0;
    while (fgets(guess,PWLENGTH, words) != NULL) {
      if(count >= nguesses) {
        return 1;
      }
      chop(guess);
      printf("%s\n", guess);
      count++;
    }
    if(count < nguesses) {
      rewind(words);
      for (int j = 0; j<6; j++) {
        count = capitalise_guess(words,count,nguesses,j);
        if(count>=nguesses)
          return 1;
        rewind(words);
      }
    }
    if(count < nguesses) {
      rewind(words);
      for (int j = 0; j<6; j++) {
        count = replace_guess_with_anychar(words,count,nguesses,j);
        if(count>=nguesses)
          return 1;
        rewind(words);
      }
    }
    return 1;
  }
// makes letter at index of element capital
int capitalise_guess(FILE * words, int count, int nguesses, int element) {
    char guess[PWLENGTH];
    while (fgets(guess,PWLENGTH, words) != NULL) {
      if(count >= nguesses)
        return count;

      chop(guess);
      if(strlen(guess) > element) {
        if(guess[element]>='a' && guess[element] <='z') {
          guess[element] = guess[element] - 32;
          printf("%s\n", guess);
          count++;
        }
      }
    }
    return count;
  }

int replace_guess_with_anychar(FILE * words, int count, int nguesses, int element) {
    char guess[PWLENGTH];
    while (fgets(guess,PWLENGTH, words) != NULL) {
      if(count >= nguesses)
        return count;
      chop(guess);
      if(strlen(guess) > element) {
        for(int n = 32; n<127; n++) {
          guess[element] = n;
          printf("%s\n", guess);
          count++;
        }
      }
    }
  return count;
}
