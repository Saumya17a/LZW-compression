#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define DICTSIZE 4096 /* allow 4096 entries in the dict  */
#define ENTRYSIZE 32

unsigned char dict[DICTSIZE][ENTRYSIZE]; /* of 30 chars max; the first byte */
                                         /* is string length; index 0xFFF   */
                                         /* will be reserved for padding    */
                                         /* the last byte (if necessary)    */

// These are provided below
int read12(FILE *infil);
int write12(FILE *outfil, int int12);
void strip_lzw_ext(char *fname);
void flush12(FILE *outfil);
int in_dict(unsigned char array[]);
int k_in_dict(int k);
void dictionary_init();
void encode(FILE *in, FILE *out);
void decode(FILE *in, FILE *out);
void set_to_wk(unsigned char to[],unsigned char from[]);
int search_for_index(unsigned char array[]);

int main(int argc, char *argv[])
{
   dictionary_init();
   FILE *write;
   FILE *readonly;

   if (argc != 3 || (argv[2][0] != 'e' && argv[2][0] != 'd'))
   {
      printf("Invalid Usage, expected: LZW {input_file} [e | d]");
      exit(4);
   }

   /*Checks whether filename is given or not*/
   if (strcmp(argv[1], "") == 0)
   {

      printf("Error: No input file specified!");
      exit(1);
   }

   else
   {

      if (strcmp(argv[2], "e") == 0)
      {
         // char new[strlen(argv[1])+5];
         // new = strcat(new , ".LZW");
         // write = fopen(new, 'w');
         // readonly = fopen(argv[1], 'r');
         char new[strlen(argv[1]) + 5];
         readonly = fopen(argv[1], "rb");
         strncpy(new, argv[1], strlen(argv[1]) + 5);

         strcat(new, ".LZW");
         write = fopen(new, "wb");
         encode(readonly, write);
      }

      else if (strcmp(argv[2], "d") == 0)
      {
         char dnew[strlen(argv[1]) - 5];
         readonly = fopen(argv[1], "rb");
         strip_lzw_ext(argv[1]);
         write = fopen(argv[1], "wb");
         decode(readonly, write);
      }
   }

   fclose(readonly);
   fclose(write);
   return 0;
}

void print(unsigned char w[])
{
   for (int i = 1; i <= w[0]; i++)
      printf("%d ", w[i]);
   printf("\n");
}

/*****************************************************************************/
/* encode() performs the Lempel Ziv Welch compression from the algorithm in  */
/* the assignment specification. The strings in the dictionary have to be    */
/* handled carefully since 0 may be a valid character in a string (we can't  */
/* use the standard C string handling functions, since they will interpret   */
/* the 0 as the end of string marker). Again, writing the codes is handled   */
/* by a separate function, just so I don't have to worry about writing 12    */
/* bit numbers inside this algorithm.                                        */
void encode(FILE *in, FILE *out)
{
   int counter = 256;
   // TODO implement
   unsigned char w[ENTRYSIZE];
   w[0] = 0;

   int f=0;
   unsigned char k;
   int index = 0;
   int flag = 0;
   while (fread(&k, sizeof(k), 1, in))
   {  
      // if (f<5){
      //    printf("%d ",k);
      //    f++;
      // }
      if (w[0] == ENTRYSIZE - 1)
      {
         index = search_for_index(w);
         write12(out, index);
         w[0] = 0;
      }
      
      w[0]++;
      w[w[0]] = k;
      flag = in_dict(w);
      if (flag == 1)
      {
         flag = 0;
      }
      else
      {
         int s;
         w[0]--;
         index = search_for_index(w);
         // if (f<5){
         //    printf("%d\n",index);
         // }
         write12(out, index);
         w[0]++;
         if (counter < 4095)
         {
            for (s = 0; s <= w[0]; s++)
            {
               dict[counter][s] = w[s];
            }
            // printf("Cursor %d ", counter);
            // print(w);

            counter++;
         }
         w[0] = 1;
         w[1] = k;
      }
   }

   index = search_for_index(w);
   write12(out, index);

   flush12(out);

   return;
}

/*****************************************************************************/
/* decode() performs the Lempel Ziv Welch decompression from the algorithm   */
/* in the assignment specification.                                          */
void decode(FILE *in, FILE *out)
{
   // TODO implement
   int read;
   int counter = 256;
   int i;
   int countstr;
   int flag;
   unsigned char str[32];
   unsigned char k;

   unsigned char w[32] = "";
   read = read12(in);
   // str = dict[read];
   for (i = 0; i <= dict[read][0]; i++)
   {
      str[i] = dict[read][i];
      // countstr++;
   }
   for (int j = 1; j <= dict[read][0]; j++)
   {
      fwrite(&str[j], 1, 1, out);
   }
   
   set_to_wk(w, dict[read]);
   while (1)
   {
      
      read = read12(in);
      if (read == -1 || read == 4095)
      {
         break;
      }
      flag = k_in_dict(read);
      if (flag == 1)
      {
         int j;
         for (j = 1; j <= dict[read][0]; j++)
         {
            fwrite(&dict[read][j], 1, 1, out);
         }
         if (w[0] < ENTRYSIZE - 1)
         {
            w[0]++;
            w[w[0]] = dict[read][1];
            if (counter < 4095){
               int x;
               for (x = 0; x <= w[0]; x++)
               {
                  dict[counter][x] = w[x];
               }
               counter++;
            }
         }
      }
      else
      {
         if (w[0] < ENTRYSIZE -1)
         {
            if (counter< 4095){
               w[0]++;
               w[w[0]] = w[1];
               
               int x = 0;
               for (x = 0; x <= w[0]; x++)
               {
                  dict[counter][x] = w[x];
               }
               counter++;
            }
         }
         int j;
         for (j = 1; j <= w[0]; j++)
         {
            fwrite(&w[j], 1, 1, out);
         }

      }
      set_to_wk(w, dict[read]);
      
   }
}

void set_to_wk(unsigned char to[],unsigned  char from[])
{
   int i;
   for (i = 0; i <= from[0]; i++)
   {
      to[i] = from[i];
   }
}

int k_in_dict(int k)
{
   // int i;
   // int flag = 1;
   // for(i =0; i<= 4095; i++){
   //    if(dict[i][0] != 0 ){
   //       return 1;
   //    }

   //    else{
   //       flag =0;
   //    }
   // }
   return (dict[k][0] != 0);
}

int search_for_index(unsigned char array[])
{
   int i;
   int j;
   // if (f<5){
   //    print(array);
   // }
   int f = 0;
   
   for (i = 0; i <= 4095; i++)
   {
      f = 1;
      for (j = 0; j <= array[0]; j++)
      {
         if (array[j] != dict[i][j])
         {
            f = 0;
         }
      }
      if (f == 1)
      {
         return i;
      }
   }
}

void dictionary_init()
{
   int value;
   for (value = 0; value <= 255; value++)
   {
      dict[value][0] = 1;
      dict[value][1] = value;
   }
   for (value = 256; value < 4096; value++)
   {
      dict[value][0] = 0;
   }
}

int in_dict(unsigned char array[])
{
   int f = 0;
   int  i;
   int j;
   for (i = 0; i <= 4095; i++)
   {
      f = 1;
      for (j = 1; j <= array[0]; j++)
      {
         if (array[j] != dict[i][j])
         {
            f = 0;
            break;
         }
      }
      if (f == 1)
      {
         return 1;
      }
   }
   return 0;
}

/*****************************************************************************/
/* read12() handles the complexities of reading 12 bit numbers from a file.  */
/* It is the simple counterpart of write12(). Like write12(), read12() uses  */
/* static variables. The function reads two 12 bit numbers at a time, but    */
/* only returns one of them. It stores the second in a static variable to be */
/* returned the next time read12() is called.                                */
int read12(FILE *infil)
{
   static int number1 = -1, number2 = -1;
   unsigned char hi8, lo4hi4, lo8;
   int retval;

   if (number2 != -1)   /* there is a stored number from   */
   {                    /* last call to read12() so just   */
      retval = number2; /* return the number without doing */
      number2 = -1;     /* any reading                     */
   }
   else /* if there is no number stored    */
   {
      if (fread(&hi8, 1, 1, infil) != 1) /* read three bytes (2 12 bit nums)*/
         return (-1);
      if (fread(&lo4hi4, 1, 1, infil) != 1)
         return (-1);
      if (fread(&lo8, 1, 1, infil) != 1)
         return (-1);

      number1 = hi8 * 0x10;                /* move hi8 4 bits left            */
      number1 = number1 + (lo4hi4 / 0x10); /* add hi 4 bits of middle byte    */

      number2 = (lo4hi4 % 0x10) * 0x0100; /* move lo 4 bits of middle byte   */
                                          /* 8 bits to the left              */
      number2 = number2 + lo8;            /* add lo byte                     */

      retval = number1;
   }

   return (retval);
}

/*****************************************************************************/
/* write12() handles the complexities of writing 12 bit numbers to file so I */
/* don't have to mess up the LZW algorithm. It uses "static" variables. In a */
/* C function, if a variable is declared static, it remembers its value from */
/* one call to the next. You could use global variables to do the same thing */
/* but it wouldn't be quite as clean. Here's how the function works: it has  */
/* two static integers: number1 and number2 which are set to -1 if they do   */
/* not contain a number waiting to be written. When the function is called   */
/* with an integer to write, if there are no numbers already waiting to be   */
/* written, it simply stores the number in number1 and returns. If there is  */
/* a number waiting to be written, the function writes out the number that   */
/* is waiting and the new number as two 12 bit numbers (3 bytes total).      */
int write12(FILE *outfil, int int12)
{
   static int number1 = -1, number2 = -1;
   unsigned char hi8, lo4hi4, lo8;
   unsigned long bignum;

   if (number1 == -1) /* no numbers waiting             */
   {
      number1 = int12; /* save the number for next time  */
      return (0);      /* actually wrote 0 bytes         */
   }

   if (int12 == -1)     /* flush the last number and put  */
      number2 = 0x0FFF; /* padding at end                 */
   else
      number2 = int12;

   bignum = number1 * 0x1000; /* move number1 12 bits left      */
   bignum = bignum + number2; /* put number2 in lower 12 bits   */

   hi8 = (unsigned char)(bignum / 0x10000);               /* bits 16-23 */
   lo4hi4 = (unsigned char)((bignum % 0x10000) / 0x0100); /* bits  8-15 */
   lo8 = (unsigned char)(bignum % 0x0100);                /* bits  0-7  */

   fwrite(&hi8, 1, 1, outfil); /* write the bytes one at a time  */
   fwrite(&lo4hi4, 1, 1, outfil);
   fwrite(&lo8, 1, 1, outfil);

   number1 = -1; /* no bytes waiting any more      */
   number2 = -1;

   return (3); /* wrote 3 bytes                  */
}

/** Write out the remaining partial codes */
void flush12(FILE *outfil)
{
   write12(outfil, -1); /* -1 tells write12() to write    */
} /* the number in waiting          */

/** Remove the ".LZW" extension from a filename */
void strip_lzw_ext(char *fname)
{
   char *end = fname + strlen(fname);

   while (end > fname && *end != '.' && *end != '\\' && *end != '/')
   {
      --end;
   }
   if ((end > fname && *end == '.') &&
       (*(end - 1) != '\\' && *(end - 1) != '/'))
   {
      *end = '\0';
   }
}
