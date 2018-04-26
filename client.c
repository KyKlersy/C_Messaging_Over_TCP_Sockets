
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "binn.h"

//C89 Defines for bool type.
typedef enum { false, true } bool;

//Def for buffer size
#ifndef INPUTBUFFERSIZE
#define INPUTBUFFERSIZE 4096
#endif

//Message stuct for sending fixed width size to server as header.
// 4 bytes, 32bits fixed.
struct messageHeader {
  uint32_t objectSize;
};

//My lazy def function for not having to copy paste a specific print for erros.
void printErrorMsg(char *errmsg);

//Function for getting variable string input of any length [[Assuming you don't out of memory]]
//Takes a C-String as input to prompt as text to the user.
//Returns a pointer to a malloc'd string the user input, this will need to be free'd later.
char* getInput(char *prompt);

//This function checks the last two characters of the string input for the tokens
//ZZ if they are found it truncates the string replacing the first Z with the null terminator.
//Returns a boolean value True if ZZ or false. This value is packed into the binn object and used
//on the server as a control signal.
bool checkProgramExit(char *input, size_t stringlength);

//Main requires arguments of ./client 127.0.0.1 41456
// In order of program name, host, port number.
int main(int argc, char const *argv[])
{
  //Check for arguments passed, must be atleast 3 for ./client/host ip/port
  if(argc < 3)
  {
    printErrorMsg("Not Enough Arguments Passed in form of [./client] [Ip Address] [Port #] \n");
    return(-1);
  }

  int sock = 0;
  struct sockaddr_in server_address;

  //Try and create an ip4 tcp socket file descriptor, if the return fails print
  //error message and quit.
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printErrorMsg("Unable to create Socket\n");
    close(sock);
    return(-1);
  }

  //zero out the server_address struct
  memset(&server_address, '0', sizeof(server_address));

  //Try and convert given host address for the server from string
  //to binary form else print error message and quit.
  if((inet_pton(AF_INET, argv[1], &server_address.sin_addr)) <= 0)
  {
    printErrorMsg("Failed to convert given address to ip4 binary form.\n");
    close(sock);
    return(-1);
  }

  //set server address options
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(atoi(argv[2]));

  //Try connecting to the server.
  if(connect(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
  {
    printErrorMsg("Unable to establish connection.\n");
    close(sock);
    return(-1);
  }

  bool progamExitFlag = false;
  char *userInput = 0;
  size_t sizeofstring = 0;

  //Binary Serialization object
  binn *obj;

  //loop until user enters ZZ token.
  do
  {
    //Fetch variable length user input string.
    userInput = getInput("Please enter input to send to server >> ");
    if(userInput == NULL)
    {
      printErrorMsg("Out of memory, Nope.jpg");
      return (-1);
    }

    //Length of input.
    sizeofstring = strlen(userInput);
    //Check for quit program tokens ZZ, flag true if found else false.
    progamExitFlag = checkProgramExit(userInput, sizeofstring);

    //Construct binary object, pack with flag and user input string.
    obj = binn_object();
    binn_object_set_bool(obj, "TC", progamExitFlag);
    binn_object_set_str(obj, "Message", userInput);

    //Construct struct for sending message header.
    struct messageHeader header;
    header.objectSize = htonl(binn_size(obj));

    //printf("uint32: %" PRIu32 "  %d : header size,  Binn Object size: %d\n", header.objectSize, sizeof(header), binn_size(obj));
    //Debug statment above ignore, send over socket the header to the server so that
    //the server can understand it's reading a fixed width of 4bytes to get the size of the payload.
    if( send(sock, &header, sizeof(header), 0) < 0 )
    {
      printErrorMsg("Send Failed");
      close(sock);
      return(-1);
    }

    //Send the binary packed object containing message and quit program flag.
    if( send(sock, binn_ptr(obj), binn_size(obj), 0) < 0 )
    {
      printErrorMsg("Send Failed");
      close(sock);
      return(-1);
    }

    //Dynamic allocations and memory cleanup.
    free(userInput);
    userInput = NULL;
    binn_free(obj);
    obj = NULL;

  } while(progamExitFlag != true);

  //Close the socket.
  close(sock);

  return 0;
}

/*Function definitions below*/

void printErrorMsg(char *errmsg)
{
  fprintf(stderr, "%s\n", errmsg);
  fprintf(stderr, "Program will now exit... \n" );
}

/****************************************************************
 *Big Kahuna, this function allows for any size input, until you*
 *run out of cpu memory returns a char pointer to the input     *
 ****************************************************************/
char* getInput(char *prompt)
{

  char *input = 0;
  size_t current_size = 0;
  size_t current_max = INPUTBUFFERSIZE;

  input = malloc(INPUTBUFFERSIZE);
  current_size = current_max;

  printf("%s", prompt);
  fflush(stdin);
  fflush(stdin);

  //Check for malloc initial not null.
  if(input != NULL)
  {
    int chr = EOF;
    unsigned int index = 0;

    //Loop until new line or end of stdin.
    while ((( chr = getchar()) != '\n') && (chr != EOF))
    {
      //Copy character into the input buffer.
      input[index++] = (char)chr;

      //Post-update check on index to see if now outside of memory bound.
      if(index == current_size)
      {
        //Double the size [4096 * 2] starting, double each time out of room.
        current_size = index + (current_size * 2);

        //Realloc for more space, checking to make sure success
        char *extrabuffer = realloc(input, current_size);

        //If not null, success, otherwise FUCK (Out of memory)
        //and since i am not getting paid, don't care to handle.
        if(extrabuffer)
        {
          input = extrabuffer;
        }
        else
        {
          //Hope and pray this is not a mission critical piece.
          printf("The world has come to an end and no more memory exists\n");
          break;
        }
      }
    }
    //Reached the end make sure to properly null terminate string.
    input[index] = '\0';
  }
  else
  {
    free(input);
    return NULL;
  }

  //return user input string pointer.
  return input;

}

/****************************************************************
 *This function checks for the last two characters in the input *
 *If they are both ZZ then it returns true, else false.         *
 ****************************************************************/
bool checkProgramExit(char *input, size_t stringlength)
{

  if(input[stringlength - 2] == 'Z' && input[stringlength - 1] == 'Z' )
  {
    input[stringlength - 2] = '\0';
    return true;
  }

  return false;
}
