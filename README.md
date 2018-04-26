# C_Messaging_Over_TCP_Sockets
School Assignment to send messages over a TCP socket using Client / Server pattern.

This project makes use of an external library for binary object Serialization
This library is called binn, both the header and implementation have been included.

The project can be found at: [https://github.com/liteserver/binn](https://github.com/liteserver/binn)

Provided is the source files along with a makefile.
The makefile by default calling make, prints out a help with a list of options.

Key interest was taken when doing this assignment into creating a system that allowed semi-variable length messages to be sent over raw tcp-sockets. Many other students who also did this assignment did so using fixed width message sizes, this program however does not. It instead combines the method used by C++ vectors for getting variable length input and the power of the Binn library to serialze the text into proper network format before transmitting to the server.

Use:

```make all```


To compile both the client and server.


It is recommended to run the client and server in separate terminals as no
signal waiting is done so both client and server will print over each other.


Run Client in a Terminal / Server in another.
No brackets, just spaces replace bracket sections with information specified.

```./client [Host IP] [Port]```


```./server [Port]```

***
Server will print any errors while attempting to bind to port.
On successful setup will print "Waiting for connection..."

On client startup and successful connection, server will print
"Connection Accepted"

Client will begin prompting for input, on enter will construct, Serialize and
send the message to the server.

The server will echo out two things, the flag value set for if the client
entered ZZ, this value is ether 0 - False or 1 - True. Upon true server will
shutdown.
The other thing written out to console is the message received.

Client will Loop until "ZZ" is entered at the end of an input.
"ZZ" / "This Message ZZ" / "ThisTestZZ" will all cause the server to terminate.

***
This program supports messages up-to a length 4,294,967,295 bytes or one uint32.
No handling has been implemented for when a message length overflows uint32.
But if you are really trying to break this program, by all means try and send 4gb+ of data through console.



