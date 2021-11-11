// The MIT License (MIT)
//
// Copyright (c) 2020 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


/*
Name: Matthew Irvine
ID: 1001401200
Replacement Assignment
Description:
    Takes a reference string from datafile.txt and applies Optimal, MRU, LRU, and FIFO algorithms
    Then prints out the number of page faults each algorithm causes.
...
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_LINE 1024

typedef struct String
{
    char* entry[MAX_LINE];
    int size;
}string;

void init(string* s)
{
    s->size = MAX_LINE;
    int i = 0;
    for (; i < s->size; i++)
    {
        s->entry[i] = (char*) malloc(MAX_LINE);
        strncpy(s->entry[i], "\0", strlen("\0"));
    }
}

void init_e(string* s, int size, char* in)
{
    s->size = size;
    int i = 0;
    for (; i < s->size; i++)
    {
        s->entry[i] = (char*)malloc(MAX_LINE);
        strncpy(s->entry[i], in, strlen(in));
    }
}

void delString(string* s)
{
    int i = 0;
    for (; i < s->size; i++)
    {
        free(s->entry[i]);
    }
}

void removeSpaces(string* s, char* line)
{
    //we are going to go through an pickup anything that isn't a space.
    char* tmp = strtok(line, " ");//skip the first number
    int counter = 0;
    while (1)
    {
        tmp = strtok(NULL, " ");
        if (tmp == NULL)
            break;
        strncpy(s->entry[counter], tmp, strlen(tmp));
        counter++;
    }
}

//finds the the first occurance of token within a list of tokens
//This is used by all algorithms to check if a token already occurs in the token list.
//Fifo heavily uses this.
int findOccurance(string tokens, char* token)
{
    int i = 0;
    for (; i < tokens.size; i++)
    {
        if (strncmp(tokens.entry[i], token, 1) == 0)
        {
            return i;
        }
    }
    return -1;
}

int furthestAway(string tokens, string s, int lineSize, int start)
{
    int savePos[tokens.size];
    //had to do this because memset would not work properly.
    int x = 0;
    for (; x < tokens.size; x++)
    {
        savePos[x] = -1;
    }
    int i = 0;
    for (; i < s.size; i++)
    {
        int y = 0;
        //printf("Starting at: %d\n", start);
        //skip any before start and anything that is a space.
        if (i < start || strncmp(s.entry[i], " ",1) == 0)
            continue;
        for (; y < tokens.size; y++)
        {
            if (strncmp(tokens.entry[y], s.entry[i],strlen(s.entry[i])) == 0)
            {
               //if any position in tokens is equal to a spot in entry then record the spot (i)
                //ONLY SAVE THE FIRST FIND
                if(savePos[y] == -1)
                    savePos[y] = i;
            }
        }
    }

    //when savePos is set the sift for either a -1 or the largest numbers, -1 takes priority.
        //-1 takes priority because it represets that the number no longer occurs in the line.
        //max is used for comparison, result is used to keep trace of the position of the max
    int max = 0;
    int result = 0;
    int z = 0;
    for (; z < tokens.size; z++)
    {
        if (savePos[z] == -1)//if a -1 occurs immediately return it
        {
            return z;
        }
        else if (savePos[z] > max)//finds the maximum or furthest away a page will be used.
        {
            result = z;
            max = savePos[z];
        }
    }
    return result;  //if no -1 then return the furthest.
}

//returns the position with the highest number (i.e. the longest time without being used)
int leastUsed(int lru[], int size)
{
    int n = 0;
    int pos = 0;
    int i = 0;
    for (; i < size; i++)
    {
        if (lru[i] > n)
        {
            n = lru[i];
            pos = i;
        }
    }

    return pos;
}

//returns the position with the lowest number (i.e. the shortest time without being used)
int mostUsed(int mru[], int size)
{
    int i = 0;
    int pos = 0;
    int n = INT_MAX;
    //find the max to start to find the min.
    for (; i < size; i++)
    {
        if (n > mru[i])
        {
            n = mru[i];
            pos = i;
        }
    }

    return pos;
}

int main( int argc, char * argv[] ) 
{
  char * line = NULL;
  size_t line_length = MAX_LINE;
  char * filename;

  FILE * file;

  if( argc < 2 )
  {
    printf("Error: You must provide a datafile as an argument.\n");
    printf("Example: ./fp datafile.txt\n");
    exit( EXIT_FAILURE );
  }

  filename = ( char * ) malloc( strlen( argv[1] ) + 1 );
  line     = ( char * ) malloc( MAX_LINE );

  memset( filename, 0, strlen( argv[1] + 1 ) );
  strncpy( filename, argv[1], strlen( argv[1] ) );

  printf("Opening file %s\n", filename );
  file = fopen( filename , "r");

  if ( file ) 
  {
      while (fgets(line, line_length, file))
      {
          char* token;
          //Keep a full version of the line for later
          string s;
          init(&s);
          char* tmp = malloc(line_length);
          strncpy(tmp, line, line_length);
          tmp = strtok(tmp, "\n");
          removeSpaces(&s, tmp);

          tmp = strtok(line, "\n");
          strcat(tmp, " ");

          token = strtok(tmp, " ");
          int working_set_size = atoi(token);
          char* initializer = "";
          string tokens;
          init_e(&tokens, working_set_size, initializer);
          string Otokens;
          init_e(&Otokens, working_set_size, initializer);
          string Ltokens;
          init_e(&Ltokens, working_set_size, initializer);
          int ltracker[Ltokens.size];
          memset(ltracker, 0, Ltokens.size);
          string Mtokens;
          init_e(&Mtokens, working_set_size, initializer);
          int mtracker[Mtokens.size];
          memset(mtracker, 0, Mtokens.size);
          printf("\nWorking set size: %d\n", working_set_size);

          int counter = 0;
          int notsouselesscounter = 0;
          int fifo = 0;
          int optimal = 0;
          int lru = 0;
          int mru = 0;
          char * firstin = (char*) malloc(MAX_LINE);
          while (token != NULL)
          {
              token = strtok(NULL, " ");
              notsouselesscounter++;
              counter++;
              if (token != NULL)
              {
                  printf("Request: %d\n", atoi(token));
                  // fifo
                  if (findOccurance(tokens, token) == -1) 
                  {                      
                      fifo++;
                      int p = findOccurance(tokens, initializer);
                      
                      if (p != -1)
                      {
                          strncpy(tokens.entry[p], token, strlen(token));
                          if (p == 0)
                            strncpy(firstin, tokens.entry[p], strlen(tokens.entry[p]));
                      }
                      else
                      {
                          p = findOccurance(tokens, firstin);
                          strncpy(tokens.entry[p], token, strlen(token));
                          
                          if (p == working_set_size-1)
                              strncpy(firstin, tokens.entry[0], strlen(tokens.entry[p]));
                          else
                              strncpy(firstin, tokens.entry[p + 1], strlen(tokens.entry[p]));
                      }
                  }
                  
                  //LRU
                  int occurs = findOccurance(Ltokens, initializer);
                  int curr = findOccurance(Ltokens, token);
                  if (occurs != -1)
                  {
                      lru++;
                      strncpy(Ltokens.entry[occurs], token, strlen(token));
                      //this updates ltracker making the new entry set to 0
                      //but adds 1 to everything else
                      //effectively keeping track of how long ago something was used.
                      //higher number means long since use.
                      int i = 0;
                      for (; i < Ltokens.size; i++)
                      {
                          if (i != occurs)
                              ltracker[i]++;
                          else
                              ltracker[i] = 0;
                      }
                  }
                  else if (curr == -1)
                  {
                      lru++;
                      int pos = leastUsed(ltracker, Ltokens.size);
                      strncpy(Ltokens.entry[pos], token, strlen(token));
                      int i = 0;
                      //same as one above but uses pos
                      for (; i < Ltokens.size; i++)
                      {
                          if (i != pos)
                              ltracker[i]++;
                          else
                              ltracker[i] = 0;
                      }
                  }
                  else
                  {
                      //if a token is found then just add to every spot in ltracker.
                      //except the token position, it was just referenced.
                      int i = 0;
                      for (; i < Ltokens.size; i++)
                      {
                          if (i != curr)
                              ltracker[i]++;
                          else
                              ltracker[i] = 0;
                      }
                  }

                  //MRU
                  //very similar to LRU but we replace the most recent used.
                  occurs = findOccurance(Mtokens, initializer);
                  curr = findOccurance(Mtokens, token);

                  //printf("Occures: %d\n", occurs);
                  //printf("Curr: %d\n", curr);

                  if (occurs != -1)
                  {
                      mru++;
                      strncpy(Mtokens.entry[occurs], token, strlen(token));
                      //this updates mtracker making the new entry set to 0
                      //but adds 1 to everything else
                      //effectively keeping track of how long ago something was used.
                      //higher number means longer since use.
                      int i = 0;
                      for (; i < Mtokens.size; i++)
                      {
                          if (i != occurs)
                              mtracker[i]++;
                          else
                              mtracker[i] = 0;
                      }
                  }
                  else if (curr == -1)
                  {
                      mru++;
                      int pos = mostUsed(mtracker, Mtokens.size);
                      //printf("Pos: %d\n", pos);
                      strncpy(Mtokens.entry[pos], token, strlen(token));
                      //same as one above but uses pos
                      int i = 0;
                      for (; i < Mtokens.size; i++)
                      {
                          if (i != pos)
                              mtracker[i]++;
                          else
                              mtracker[i] = 0;
                      }
                  }
                  else
                  {
                      //if a token is found then just add to every spot in ltracker.
                      //except the token position, it was just referenced.
                      int i = 0;
                      for (; i < Mtokens.size; i++)
                      {
                          if (i != curr)
                              mtracker[i]++;
                          else
                              mtracker[i] = 0;
                      }
                  }
                  //optimal
                  occurs = findOccurance(Otokens, initializer);
                  if (occurs != -1)
                  {
                      optimal++;
                      strncpy(Otokens.entry[occurs], token, strlen(token));
                  }
                  else if(findOccurance(Otokens, token) == -1)
                  {
                      optimal++;
                      int pos = furthestAway(Otokens, s, line_length, notsouselesscounter);
                      int n = 0;
                      if (strlen(Otokens.entry[pos]) > strlen(token))
                          n = strlen(Otokens.entry[pos]);
                      else
                          n = strlen(token);
                      strncpy(Otokens.entry[pos], token, n);
                  }
                  
                  if (counter == working_set_size)
                  {
                      printf("\n");
                      counter = 0;
                  }
              }
          }
          printf("Page Faults FIFO: %d\n", fifo);
          printf("Page Faults Optimal: %d\n", optimal);
          printf("Page Faults LRU: %d\n", lru);
          printf("Page Faults MRU: %d\n", mru);
          printf("\n");
          free(firstin);
          delString(&tokens);
          delString(&Ltokens);
          delString(&Mtokens);
          delString(&Otokens);
      }
      free( line );
      fclose(file);
  }

  return 0;
}
