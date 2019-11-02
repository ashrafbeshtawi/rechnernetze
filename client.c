#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc,char **argv) {

    char * buffer;
    //although the server response in RFC  should be no longer than 512 charachter but my buufer is 2 times bigger to be able to deal with unexpected responses from the server
    int buff_size=1024;
    //linked lists for the call of getaddrinfo()
    struct addrinfo * result;
    struct addrinfo    hints;
    //filling the hints object
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_family=AF_INET;
    hints.ai_protocol=0;
    hints.ai_flags=0;


    int my_socket=0;
    int connection=0;

    //error texts
    char error_arguments[]="Too few arguments\n";
    char error_connection[]="Connection could not be established\n";

    //checking if server and port are given as arguments
    if(argc<3){
    //if not given then give an error message and exit
    fwrite(error_arguments,sizeof(char),sizeof(error_arguments),stdout);
    exit(1);
    }

    //getting the address information
    int request=getaddrinfo(argv[1],argv[2],&hints,&result);

    //if getaddrinfo() failed then exit..
    if(request!=0){
        fwrite(error_connection,sizeof(char),sizeof(error_connection),stdout);
        exit(1);
    }

    //going throw the linked list and trying to ceate a connection
    for(struct addrinfo *start=result;start!=NULL;start=start->ai_next){
            //create socket
            my_socket=socket(start->ai_family,start->ai_socktype,start->ai_protocol);
            //if creating the socket failed then go to the next element in the linked list
            if(my_socket==-1){
                continue;
            }
            //create connection
            connection=connect(my_socket,start->ai_addr,start->ai_addrlen);

            //if creating the connection failed then go to the next element in the linked list
            if(connection==-1){
                continue;
            }

    }
    //free the linked list
    freeaddrinfo(result);

    //if the connection could not be established with any of the elements in the linked list then exit with error message
    if(my_socket==-1 || connection==-1){
        fwrite(error_connection,sizeof(char),sizeof(error_connection),stdout);
        //closing socket only if it was opened :D
        if(my_socket>0){
            close(my_socket);
        }
        exit(1);
    }


    //allocate memory for the buffer
    buffer=(char *) calloc(buff_size,sizeof(char));
    //as long the server sends Data then receive
    int ruckgabe=1;
    while(ruckgabe>0){
            //emptying the buffer for new saving
            memset( buffer, '\0', sizeof(char)*buff_size );
            //get message from the server and save it in the buffer
            ruckgabe=recv(my_socket,buffer,buff_size,0);
            //printing the received data
            fwrite(buffer,sizeof(char),ruckgabe,stdout);

    }
    //freeing the allocated resources and closing the socket
    free(buffer);
    close(my_socket);




    return 0;
}
