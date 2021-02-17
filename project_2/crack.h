#ifndef CRACK
#define CRACK

// had to make shorter than 10k as it wanst functioning on VM and would break
// some functions, likely due to memory problems
#define PWLENGTH 100

void chop(char *word);
void string2ByteArray(char* input, BYTE* output);
void ReadFile(char *name, char* buffer);
void checkpw(BYTE text[], char * fname, int index);
void crack4chars(char*fileName);
void brute6chars(char*fileName);
int guessing(int nguesses);
int capitalise_guess(FILE * words, int count,int nguesses, int element);
int replace_guess_with_anychar(FILE * words, int count, int nguesses, int element);
void checkpws(BYTE text[], char * fname, int index, int len, char * word);

#endif
