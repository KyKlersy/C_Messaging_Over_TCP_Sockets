#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <inttypes.h>
#include "binn.h"

//C89 Defines for bool type.
typedef enum { false, true } bool;

#ifndef READBUFFERSIZE
#define READBUFFERSIZE 8192
#endif


void printErrorMsg(char *errmsg)
{
  fprintf(stderr, "%s\n", errmsg);
  fprintf(stderr, "Program will now exit... \n" );
}

int main(int argc, char const *argv[])
{

  int server_sock, client_sock, sockLength;
  struct sockaddr_in server_address, client_address;
  char buf[READBUFFERSIZE];

  if(argc < 2)
  {
    printErrorMsg("Not Enough Arguments Passed in form of [./server] [Port # to listen on.] \n");
    return(-1);
  }

  //Create server socket
  if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printErrorMsg("Unable to create Socket\n");
    close(server_sock);
    return(-1);
  }

  //Setup Server Sock struct
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(atoi(argv[1]));

  //Bind server to listen on port entered as command argument.
  if( bind(server_sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0 )
  {
    printErrorMsg("Unable to Bind Socket\n");
    close(server_sock);
    return(-1);
  }

  //Listen for connections, buffer upto 5 messages.
  /* Could Modify this with fork and thread handlers for mutliple connection support*/
  listen(server_sock, 5);
  printf("Waiting for connection...\n");

  sockLength = sizeof(struct sockaddr_in);

  client_sock = accept(server_sock, (struct sockaddr *) &client_address, (socklen_t *) &sockLength);

  if(client_sock < 0)
  {
    printErrorMsg("Failed to Accept Client Connection\n");
    close(server_sock);
    return(-1);
  }

  printf("Connection Accepted\n");

  bool clientExitFlag = false;
  bool recvPayloadLength = false;
  char *data = 0;
  size_t messageSizeRead = 0;
  size_t bytes_received = 0;
  size_t offset = 0;

  uint32_t networkOrdered = 0;
  uint32_t payloadLength = 0;

  binn *recObj;

  do
  {
    //zero the buffer
    memset(&buf, '0', sizeof(buf));
    //8191 read, messageSizeRead 8192 is the null
    while((messageSizeRead = recv(client_sock, buf, sizeof(buf) - 1, 0 )) > 0)
    {
      //count bytes recieved
      bytes_received += messageSizeRead;

      //Beacon onto the first four bytes we know are fixed size
      //that tell the program how large the data payload is.
      if(bytes_received >= 4 && recvPayloadLength != true)
      {
        //retreive the network orderd bytes for uint32_t
        //convert back to host orderd.
        memcpy(&networkOrdered, buf, sizeof(networkOrdered));
        payloadLength = ntohl(networkOrdered);

        //initialize the data buffer since we know the payload size now.
        data = malloc(sizeof(char)*payloadLength);
        //Offset shift the buffer by 4 <<<< since 4 bytes were removed to read
        //the payload size.
        memmove(buf, buf+4, messageSizeRead);
        messageSizeRead -= 4;
        recvPayloadLength = true;
      }

      //only after the data size has been found, start copying the buffer to
      //the data buffer.
      if(recvPayloadLength == true)
      {
        memcpy((data + offset), buf, messageSizeRead);
        offset += messageSizeRead;
      }

      //if all of the data has been collected reset and break out of loop.
      if(bytes_received >= (payloadLength + 4))
      {
        payloadLength = 0;
        offset = 0;
        bytes_received = 0;
        recvPayloadLength = false;
        break;
      }

    }

    //Process the data receieved.
    recObj = binn_open(data);
    clientExitFlag = binn_object_bool(recObj, "TC");
    char *messageRec = binn_object_str(recObj, "Message");

    printf("Exit Flag: %d\n", clientExitFlag );
    printf("Message Recieved: %s\n", messageRec);

    //Cleanup dynamic allocations.
    free(data);
    binn_free(recObj);
    recObj = NULL;

    //Test and checks for if the client has left or failed to recieve a msg.
    if(messageSizeRead == 0)
    {
      clientExitFlag = true;
      printf("Client disconnected.\n");
    }
    else if( messageSizeRead == -1)
    {
      printf("Receive Failed\n");
    }

  } while(clientExitFlag != true);

  //Close server/client socket.
  close(server_sock);
  close(client_sock);

  return 0;
}
