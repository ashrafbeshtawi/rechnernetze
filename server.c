#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>


//headers for my functions
int count_lines(char * file_name);
int get_quote(char * file_name,char * buffer,int buff_size);
int is_empty(char * file_name);

int main(int argc,char **argv){
    //place holder for the result for calling socket() bind() listen()
    int my_socket,my_bind,my_listen;

    //place holder to save the client information
    struct sockaddr_in clientaddr;
    socklen_t client_size=sizeof(clientaddr);

    //linked lists for getaddrinfo()
    struct addrinfo * result;
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;

    //creating a buffer with the size of 512 charachter (RFC rules)
    int buff_size=512;
    char *buffer;

    //error texts
    char error_arguments[]="Too few arguments\n";
    char error_connection[]="Connection could not be established\n";
    char error_file[]="file is empty or bad file\n";

    //checking if server and port are given as arguments
    if(argc<3){
    //if not given then give an error message and exit
    fwrite(error_arguments,sizeof(char),sizeof(error_arguments),stdout);
    exit(1);
    }

    //checking if the file is ok
    //if the file is empty and no \n char is there then show error and close
    if(count_lines(argv[2])<=0){
        fwrite(error_file,sizeof(char),sizeof(error_file),stdout);
        exit(1);
    }


    //getting the infos of this address
    int request=getaddrinfo(NULL,argv[1],&hints,&result);
    //if getaddrinfo() failed then exit..
    if(request!=0){
        fwrite(error_connection,sizeof(char),sizeof(error_connection),stdout);
        exit(1);
    }

    //allocating memory for the buffer
    buffer=malloc(sizeof(char)*buff_size);



    //going throw the linked list and trying to create a socket and bind
    for(struct addrinfo * start=result;start!=NULL;start=start->ai_next){
        //creating socket
         my_socket=socket(AF_INET, SOCK_STREAM, 0);
            //if creating was successful then bind
            if (my_socket>=0)
            {
                //trying to bind ...
                 my_bind=bind(my_socket,start->ai_addr, start->ai_addrlen);
                //if bind was successful then listen..
                if(my_bind==0){
                     my_listen=listen(my_socket,1);
                    // if listen() successed then exit the loop
                    if(my_listen==0){
                        break;
                    }
                }
            }
            //else we keep going throw the linked list and trying...
    }
    //free the linked list
    freeaddrinfo(result);

    //if connection could not be established then error and exit
    if(my_socket<0 || my_bind<0 || my_listen<0){
        fwrite(error_connection,sizeof(char),sizeof(error_connection),stdout);
        //free the buffer
        free(buffer);
        //closing socket only if it was opened :D
        if(my_socket>0){
            close(my_socket);
        }
        exit(1);
    }

    //else we ait for clients...
    while(1){
        //accepting client
        int my_accept=accept(my_socket,(struct sockaddr *)&clientaddr,(socklen_t*) &client_size);
        //if accepting was successful then get a quote from the file and save it in the buffer (the function does all of that)
        if(my_accept>=0){
            int quote=get_quote(argv[2],buffer,buff_size);
            //if getting a quote went wrong then free the buffers and close the socket and exit(the get_quote() wil show an error message in this case)
            if(quote==-1){
            free(buffer);
            close(my_socket);
            exit(1);
            }
            //else send the quote to the client
            int my_send=send(my_accept,buffer,strlen(buffer),0);
            // if send went wrong try to repeat 10 times
            for(int i=0;i<10 && my_send<0;i++){
                my_send=send(my_accept,buffer,strlen(buffer),0);
            }
            //if send faild in all of the 10 tries then show error message and close
            if(my_send<0){
                fwrite(strerror(errno),sizeof(char),sizeof(strerror(errno)),stdout);
            }
            //closing the connection
            close(my_accept);
        }else{
            //if this client could not be accepted then show error message and wait for other clients
            fwrite(strerror(errno),sizeof(char),sizeof(strerror(errno)),stdout);
        }
    }
    //this section should never be reached due to the infinte-loop
    free(buffer);
    close(my_socket);

    return 0;
}



//retruns the number of lines in a file
int count_lines(char * file_name){
    FILE * my_file;
    my_file=fopen(file_name,"r");
    if(my_file==NULL){
            return -1;
    }

    int count=0;
    int c=getc(my_file);

    while(c!=EOF){
        if(c=='\n'){
            count++;
        }
        c=getc(my_file);
    }
    fclose(my_file);

    return count;
}

//saves a random quote from file to buffer and retruns 0 if success and -1 otherwise
int get_quote(char * file_name,char * buffer,int buff_size){
    //emptying the buffer for new saving
    memset( buffer, '\0', sizeof(char)*buff_size );
    //error message
    char empty_file[]="The selected file is empty";
    char error_quote[]="The quote is longer than 512 charachter, only the first 512 charachter were sent\n";
    FILE * my_file;
    //opening the file to read
    my_file=fopen(file_name,"r");
    //if opening failed show error message and return -1
    if(my_file==NULL){
            fwrite(strerror(errno),sizeof(char),sizeof(strerror(errno)),stdout);
            return -1;
    }
    //get the number of lines in the file
    int lines_number=count_lines(file_name);

    //if getting the number of lines failed then return -1
    if(lines_number<=0){
        fwrite(empty_file,sizeof(char),sizeof(empty_file),stdout);
        fclose(my_file);
        return -1;
    }
    //getting a random number to choose a line
    srand(time(NULL));
    int my_quote=rand()%lines_number;

    //jumping to the choosen line
    int line=0;
    char c=0;
    while(c!=-1 && line<my_quote){
        c=getc(my_file);
        if(c=='\n'){
            line++;
        }
    }
    //reading the line
    int index=0;
    c=getc(my_file);
    while(c!=-1 && c!='\n' && index<buff_size){
        buffer[index]=c;
        index++;
        c=getc(my_file);
    }

    //if the quote is longer then 512 show an error message and return the first 512 charachter only
    if(index>=buff_size){
        fwrite(error_quote,sizeof(char),sizeof(error_quote),stdout);        
    }
    //closing the file
    fclose(my_file);
    return 0;



}
