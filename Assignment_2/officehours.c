// Copyright (c) 2020 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
  
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3
#define CLASSE 4

/* TODO */
/* Add your synchronization variables here */
#define decimal 10

typedef struct
{
	char report[256];
} Message;

Message message;
pthread_mutex_t lock;
typedef sem_t Sema;
Sema * sema;
int permission = 0;

/*Function Declarations*/
void reportError(Message, int);
void report(Message);
Sema * make(int);
int semaDestory(Sema*);
int semaWait(Sema*);
int semaPost(Sema*);

/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the 
 * code that you develop. 
 */

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;


typedef struct 
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;

/* Called at beginning of simulation.  
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;

  /* Initialize your synchronization variables (and 
   * other variables you might use) here
   */
  //sema = make(MAX_SEATS);

  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL) 
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && 
           i < MAX_STUDENTS ) 
  {
    i++;
  }

 fclose(fp);
 return i;
}

void reportError(Message mes, int exitStat)
{
	perror(mes.report);
	exit(exitStat);
}

void report(Message mes)
{
	printf(mes.report);
	printf("\n");
}

void reportInt(Message mes, int i)
{
    printf(mes.report, i);
    printf("\n");
}

/*Some functions that make use of basic semaphore functions but make
 * my life a lot easier.
 *
 * the make function will allocate a semaphore for me and do checks in case
 * it failed.
 *
 * The semaWait function will do the sem_wait function and check that it succeeds
 *
 * The semaPost function will do the sem_post function and check that it succeeds
 */
Sema * make(int v)
{
	Sema *sem = malloc(sizeof(Sema));
	if (sem == NULL)
	{
		strcpy(message.report, "Error: Semaphore was not allocated!");
		reportError(message, 1);
	}

	int x = sem_init(sem, 0, v);
	if (x != 0)
	{
		strcpy(message.report, "Error: Semaphore initialization failed!");
		reportError(message, 1);
	}

	return sem;
}

int semaDestroy(Sema* sem)
{
    int x = sem_destroy(sem);
    if (x != 0)
    {
        strcpy(message.report, "Error: Semaphore failed to destroy!");
        reportError(message, 1);
    }
    return x;
}

int semaTryWait(Sema * sem)
{
	int x = sem_trywait(sem);
    return x;
}

int semaWait(Sema* sem)
{
    int x = sem_wait(sem);
	if (x != 0)
	{
		strcpy(message.report, "SemaWait:");
		report(message);
		strcpy(message.report, "Semaphore wait failed!");
		reportError(message, 1);
	}
    return x;
}

int semaPost(Sema * sem)
{
	int x = sem_post(sem);
	if (x != 0)
	{
		strcpy(message.report, "Semaphore Post Failed!");
		reportError(message, 1);

	}
    return x;
}

int semaValue(Sema* sem)
{
    int x;
    if (sem_getvalue(sem, &x) == -1)
    {
        strcpy(message.report, "Could not get value!");
        reportError(message, 1);
    }

    return x;
}

/* Code executed by professor to simulate taking a break 
 * You do not need to add anything here.  
 */
static void take_break() 
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk) 
{
  printf("The professor arrived and is starting his office hours\n");
  pthread_mutex_init(&lock, NULL);
  sema = make(MAX_SEATS);
  //printf("made Sema\n");
  /* Loop while waiting for students to arrive. */
  while (1) 
  {

    /* TODO */
    /* Add code here to handle the student's request.             */
    /* Currently the body of the loop is empty. There's           */
    /* no communication between professor and students, i.e. all  */
    /* students are admitted without regard of the number         */ 
    /* of available seats, which class a student is in,           */
    /* and whether the professor needs a break. You need to add   */
    /* all of this.                                               */
      //if the professor is taking a break you can view like they are reserving all the seats for themselves
      //Thus preventing students from entering until the break is finished.
      pthread_mutex_lock(&lock);
      permission = -1;  //prevents any further semaphore trywait calls.

      if (students_since_break >= 10)
      {
          printf("take break!\n");
          while (semaValue(sema) != 3)
          {
              sleep(1);
              /*Giving all students in office time to leave.*/
          }
          //semaDestroy(sema);
          take_break();
          //sema = make(MAX_SEATS); 
      }
      permission = 0;
      pthread_mutex_unlock(&lock);
	  
  }
  pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter() 
{

  /* TODO */
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */
  /*  YOUR CODE HERE. */
    do
    {
        if (permission == 0)
        {
            if (classb_inoffice > 0)
            {
                continue;
            }
            else
            {
                pthread_mutex_lock(&lock);
                semaWait(sema);
                students_in_office += 1;
                students_since_break += 1;
                classa_inoffice += 1;
                pthread_mutex_unlock(&lock);
                break;
            }
        }
    } while (1);

    /*
                if (students_since_break >= 10)
                continue;
            if (classb_inoffice > 0)
                continue;
    */
    /*
int n = -1;
do
{
    if (permission == 0)
    {
        n = semaTryWait(sema);
    }
} while (n != 0);
*/

    /*
    do
    {
        if(classb_inoffice > 0)
        {
            continue;
        }else
        {
            pthread_mutex_lock(&lock);
            semaWait(sema);
            students_in_office += 1;
            students_since_break += 1;
            classa_inoffice += 1;
            pthread_mutex_unlock(&lock);
            break;
        }
    } while(1);
    */
}

/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter() 
{

  /* TODO */
  /* Request permission to enter the office.  You might also want to add  */
  /* synchronization for the simulations variables below                  */
  /*  YOUR CODE HERE.                                                     */ 
    
    do
    {
        if (permission == 0)
        {
            if (classa_inoffice > 0)
            {
                continue;
            }
            else
            {
                pthread_mutex_lock(&lock);
                semaWait(sema);
                students_in_office += 1;
                students_since_break += 1;
                classb_inoffice += 1;
                pthread_mutex_unlock(&lock);
                break;
            }
        }
    } while (1);

    /*
                if (students_since_break >= 10)
                continue;
            if (classa_inoffice > 0)
                continue;
    */
    /*
   int n = -1;
   do
   {
       if (permission == 0)
       {
           n = semaTryWait(sema);
       }
   } while (n != 0);
   */
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.  
 */
static void ask_questions(int t) 
{
  sleep(t);
}


/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave() 
{
  /* 
   *  TODO
   *  YOUR CODE HERE. 
   */

    //pthread_mutex_lock(&lock);
	students_in_office -= 1;
	classa_inoffice -= 1;
	semaPost(sema);
    //pthread_mutex_unlock(&lock);
    //printf("# since break: %d\n", students_since_break);
}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave() 
{
  /* 
   * TODO
   * YOUR CODE HERE. 
   */
    //pthread_mutex_lock(&lock);
	students_in_office -= 1;
	classb_inoffice -= 1;
	semaPost(sema);
    //pthread_mutex_unlock(&lock);
    //printf("# since break: %d\n", students_since_break);
}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classa_enter();

  printf("Student %d\t from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );
  
  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d\t from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d\t from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();  

  printf("Student %d\t from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d\t from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d\t from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d\t from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();        

  printf("Student %d\t from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * GUID: 355F4066-DA3E-4F74-9656-EF8097FBC985
 */
int main(int nargs, char **args) 
{
  int i;
  int result;
  int student_type;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  if (nargs != 2) 
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0) 
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result) 
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++) 
  {

    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);
                
    student_type = random() % 2;

    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result) 
    {
      printf("officehour: thread_fork failed for student %d: %s\n", 
            i, strerror(result));
      exit(1);
    }
  }

  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++) 
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  printf("Office hour simulation done.\n");

  return 0;
}
