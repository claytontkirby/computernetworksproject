#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <iostream>
using namespace std;

string trackerFilePath;

struct TrackerFile {
	char* filename;
	char* filesize;
	char* description;
	char* md5;
	char* ip;
	char* port;
};

void createSharedFileLoc();

int setupSocketConnections();

void listenForConnections(int sockid);

void peer_handler(int sock_child);

void handle_createtracker_req(int sock_child, char* read_msg);

char* createTrackerFile(char* read_msg);

TrackerFile parseCreateTrackerMsg(char* read_msg);

void handle_list_req(int sock_child);

char* xtrct_fname(char* msg, char* something);

void handle_get_req(int sock_child, char* fname);

// void tokenize_createmsg(char* msg);

void tokenize_updatemsg(char* msg);

void handle_updatetracker_req(int sock_child);

int main(){
   int sockid;

   createSharedFileLoc();

   sockid = setupSocketConnections();

   while(1){
   	cout << "listening" << endl;
   	listenForConnections(sockid);
   }
          
} // end of main  
    
void createSharedFileLoc() {
	struct stat st = {0};
	char cwd[100];

	if(getcwd(cwd, sizeof(cwd))==NULL) {
		exit(1);
	}

	trackerFilePath = cwd;
	trackerFilePath += "/trackers";
	if(stat(trackerFilePath.c_str(), &st)) {
		mkdir(trackerFilePath.c_str(), 0700);
	}
}

int setupSocketConnections() {
   struct sockaddr_in server_addr;
   int sockid;
   int server_port=5000;

   if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0){//create socket connection oriented
	   printf("socket cannot be created \n"); exit(0); 
   }
    
   //socket created at this stage
   //now associate the socket with local port to allow listening incoming connections
   server_addr.sin_family = AF_INET;// assign address family
   server_addr.sin_port = htons(server_port);//change server port to NETWORK BYTE ORDER
   server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    
   if (bind(sockid ,(struct sockaddr *) &server_addr, sizeof(server_addr)) ==-1){//bind and check error
	   printf("bind  failure\n"); exit(0); 
   }
    
   printf("Tracker SERVER READY TO LISTEN INCOMING REQUEST.... \n");
   if (listen(sockid, 1) < 0){ //(parent) process listens at sockid and check error
	   printf(" Tracker  SERVER CANNOT LISTEN\n"); exit(0);
   }  

   return sockid;                                      
}

void listenForConnections(int sockid) {
	int sockchild;
	struct sockaddr_in client_addr;
	pid_t pid;

	if ((sockchild = accept(sockid ,(struct sockaddr *) &client_addr, (socklen_t*) &client_addr ))==-1){ /* accept connection and create a socket descriptor for actual work */
		   printf("Tracker Cannot accept...\n"); exit(0); 
	}

	// if ((pid=fork())==0){//New child process will serve the requester client. separate child will serve separate client
	   close(sockid);   //child does not need listener
	   peer_handler(sockchild);//child is serving the client.		   
	   close (sockchild);// printf("\n 1. closed");
	   exit(0);         // kill the process. child process all done with work
    // }

	close(sockchild);  // parent all done with client, only child will communicate with that client from now
}

void peer_handler(int sock_child){ // function for file transfer. child process will call this function     
    //start handiling client request	
	int length;
	char read_msg[100];
	char* fname = NULL;
	int MAXLINE = 10;	
	length=read(sock_child, &read_msg, 100);
	read_msg[length]='\0';

	cout << read_msg << endl;

	if((!strcmp(read_msg, "REQ LIST"))||(!strcmp(read_msg, "req list"))||(!strcmp(read_msg, "<REQ LIST>"))||(!strcmp(read_msg, "<REQ LIST>\n"))){//list command received
		// handle_list_req(sock_child);// handle list request
		printf("list request handled.\n");
	}
	else if((strstr(read_msg,"get")!=NULL)||(strstr(read_msg,"GET")!=NULL)){// get command received
		fname = xtrct_fname(read_msg, " ");// extract filename from the command		
		// handle_get_req(sock_child, fname);
	}
	else if((strstr(read_msg,"createtracker")!=NULL)||(strstr(read_msg,"Createtracker")!=NULL)||(strstr(read_msg,"CREATETRACKER")!=NULL)){// get command received
		// tokenize_createmsg(read_msg);
		handle_createtracker_req(sock_child, read_msg);
		
	}
	else if((strstr(read_msg,"updatetracker")!=NULL)||(strstr(read_msg,"Updatetracker")!=NULL)||(strstr(read_msg,"UPDATETRACKER")!=NULL)){// get command received
		// tokenize_updatemsg(read_msg);
		// handle_updatetracker_req(sock_child);		
	}
	
}//end client handler function

void handle_list_req(int sock_child) {

}

void handle_createtracker_req(int sock_child, char* read_msg) {
	char* msg;

	msg = createTrackerFile(read_msg);

	cout << msg << endl;

	if((write(sock_child, msg, 100)) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(0);
	}
}

char* createTrackerFile(char* read_msg) {
	TrackerFile tf = parseCreateTrackerMsg(read_msg);
	FILE *fp;
	char* err = "<createtracker fail>";

	fp = fopen((trackerFilePath + "/" + tf.filename + ".txt").c_str(), "r");

	if(fp) {
		return "<createtracker ferr>";
	} else {
		fp = fopen((trackerFilePath + "/" + tf.filename + ".txt").c_str(), "w");
	}	

	if(fputs((tf.filename), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.filesize), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.description), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.md5), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.ip), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.port), fp) == EOF) { return err;}

	// fflush(fp);
	fclose(fp);

	return "<createtracker succ>";
}

TrackerFile parseCreateTrackerMsg(char* read_msg) {
	char* msg = read_msg;
	TrackerFile tf;

	strtok(msg, " ");
	tf.filename = strtok(NULL, " ");
	tf.filesize = strtok(NULL, " ");
	tf.description = strtok(NULL, " ");
	tf.md5 = strtok(NULL, " ");
	tf.ip = strtok(NULL, " ");
	tf.port = strtok(NULL, " ");

	return tf;
}

char* xtrct_fname(char* read_msg, char* something) {
	char* msg = read_msg;
	char* fname = (char *)malloc(100);
	
	sscanf(msg,"%*s %s", fname);

	return fname;
}

void handle_get_req(int sock_child, char* fname) {

}

// void tokenize_createmsg(char* msg) {

//}

void tokenize_updatemsg(char* msg) {

}

void handle_updatetracker_req(int sock_child) {

}
