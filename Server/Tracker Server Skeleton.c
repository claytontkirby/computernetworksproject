#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <fcntl.h>
#include <iostream>
#include <ifaddrs.h>
// #include <linux/if_link.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <vector>
using namespace std;

string trackerFilePath;
string sharedFilePath;
int MAX_SEND_LENGTH = 1024;
time_t timer;
struct tm newyear;

struct PeerInfo {
	string ip;
	string port;
	string start_byte;
	string end_byte;
	string timestamp;
};

struct TrackerFile {
	string filename;
	string filesize;
	string description;
	string md5;
	vector<PeerInfo> peerlist;
};

vector<TrackerFile> trackerFiles;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

void createFileDirectories();

void loadTrackerFiles();

void setupTimer();

int setupSocketConnections();

void listenForConnections(int sockid);

void peer_handler(int sock_child);

void handle_createtracker_req(int sock_child, char* read_msg);

string createTrackerFile(char* read_msg);

TrackerFile parseCreateTrackerMsg(char* read_msg);

void handle_list_req(int sock_child);

void handle_download(int sock_child, char* read_msg);

string parseGetRequest(char* read_msg);

void handle_get_req(int sock_child, char* read_msg);

void handle_updatetracker_req(int sock_child, char* read_msg);

string updateTrackerFile(char* read_msg);

int parseUpdateTrackerMsg(char* read_msg);

bool writeTrackerFile(TrackerFile tf);

int main(){
	int sockid;
	int i, family, s;
	struct ifaddrs *ifa, *ifaddr;
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	char host[NI_MAXHOST];
	bzero(host, NI_MAXHOST);

	system("clear");

	setupTimer();

	createFileDirectories();

	loadTrackerFiles();

	sockid = setupSocketConnections();	

	if(getsockname(sockid, (struct sockaddr *)&sin, &len) == -1) {
		cout << "Error getting port from socket" << endl;
	} else {
		cout << "Port: " << ntohs(sin.sin_port) << endl;
	}

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(0);
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 

	getifaddrs(&ifaddr);
	cout << "Tracker server network info..." << endl;
	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		family = ifa->ifa_addr->sa_family;
		s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, 100, NULL, 0, NI_NUMERICHOST);
		cout << "IP: " << host << endl;
	}

	while(true){
		cout << endl;
		cout << "Tracker server listening for incoming connections..." << endl;
		
		if (listen(sockid, 2) < 0){ //(parent) process listens at sockid and check error
			printf(" Tracker  SERVER CANNOT LISTEN\n"); exit(0);
		}
		listenForConnections(sockid);
	}         
}
    
void createFileDirectories() {
	char cwd[100];
	struct stat st = {0};

	if(getcwd(cwd, sizeof(cwd))==NULL) {
		exit(1);
	}

	sharedFilePath = cwd;
	trackerFilePath = cwd;
	sharedFilePath += "/shared";
	trackerFilePath += "/trackers";
	if(stat(sharedFilePath.c_str(), &st)) {
		mkdir(sharedFilePath.c_str(), 0700);
	}

	if(stat(trackerFilePath.c_str(), &st)) {
		mkdir(trackerFilePath.c_str(), 0700);
	}
	sharedFilePath += "/";
	trackerFilePath += "/";
}

void loadTrackerFiles() {
	DIR* FD;
	struct dirent* in_file;

	if(NULL == (FD = opendir(trackerFilePath.c_str()))) {
		cout << "error" << endl;
	}

	while((in_file = readdir(FD))) {
		if(in_file->d_name[0] != '.') {
			char temp[100];
			TrackerFile tf;
			PeerInfo pi;
			ifstream in;	
			strcpy(temp, trackerFilePath.c_str());
			strcat(temp, in_file->d_name);
			if(in.good()) {
				in.open(temp);
			}

			getline(in, tf.filename);
			getline(in, tf.filesize);
			getline(in, tf.description);
			getline(in, tf.md5);
			getline(in, pi.ip, ':');
			getline(in, pi.port, ':');
			getline(in, pi.start_byte, ':');
			getline(in, pi.end_byte, ':');
			getline(in, pi.timestamp);
			tf.peerlist.push_back(pi);
			trackerFiles.push_back(tf);

			in.close();
		}
	}
}

void setupTimer() {
	newyear = *localtime(&timer);
	newyear.tm_mday = 1;
	newyear.tm_mon = 0;
	newyear.tm_hour = 0;
	newyear.tm_min = 0;
	newyear.tm_sec = 0;
}

int setupSocketConnections() {
   // struct sockaddr_in server_addr;
   int sockid;
   // int server_port=5001;

   if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0){//create socket connection oriented
	   printf("socket cannot be created \n"); exit(0); 
   }
    
   //socket created at this stage
   //now associate the socket with local port to allow listening incoming connections
   server_addr.sin_family = AF_INET;// assign address family
   server_addr.sin_port = htons(0);//change server port to NETWORK BYTE ORDER
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   

   if (bind(sockid ,(struct sockaddr *) &server_addr, sizeof(server_addr)) ==-1){//bind and check error
	   printf("bind  failure\n"); exit(0); 
   }  

   return sockid;                                      
}

void listenForConnections(int sockid) {
	int sockchild;
	// pid_t pid;

	if ((sockchild = accept(sockid ,(struct sockaddr *) &client_addr, (socklen_t*) &client_addr ))==-1){ /* accept connection and create a socket descriptor for actual work */
		   printf("Tracker Cannot accept...\n"); exit(0); 
	}

	// if ((pid=fork())==0) {//New child process will serve the requester client. separate child will serve separate client
	   // close(sockid);   //child does not need listener
	   peer_handler(sockchild);//child is serving the client.	
	   cout << "handled" << endl;	   
	   close(sockchild);// printf("\n 1. closed");
	   // exit(0);         // kill the process. child process all done with work
    // }

	// close(sockchild);  // parent all done with client, only child will communicate with that client from now
}

void peer_handler(int sock_child){ // function for file transfer. child process will call this function     
    //start handiling client request	
	int length;
	char read_msg[201];
	bzero(read_msg, 201);
	length=read(sock_child, &read_msg, 200);
	read_msg[length+1]='\0';

	cout << "Message received: " << read_msg << endl;

	if((!strcmp(read_msg, "REQ LIST"))||(!strcmp(read_msg, "req list"))||(!strcmp(read_msg, "<REQ LIST>"))||(!strcmp(read_msg, "<REQ LIST>\n"))){//list command received
		handle_list_req(sock_child);// handle list request
	}
	else if((strstr(read_msg,"get")!=NULL)||(strstr(read_msg,"GET")!=NULL)){// get command received
		handle_get_req(sock_child, read_msg);
	}
	else if((strstr(read_msg,"createtracker")!=NULL)||(strstr(read_msg,"Createtracker")!=NULL)||(strstr(read_msg,"CREATETRACKER")!=NULL)){// get command received
		handle_createtracker_req(sock_child, read_msg);		
	}
	else if((strstr(read_msg,"updatetracker")!=NULL)||(strstr(read_msg,"Updatetracker")!=NULL)||(strstr(read_msg,"UPDATETRACKER")!=NULL)){// get command received
		handle_updatetracker_req(sock_child, read_msg);		
	} else if(strstr(read_msg, "download") != NULL) {
		handle_download(sock_child, read_msg);
	}
	
}//end client handler function

void handle_createtracker_req(int sock_child, char* read_msg) {
	string msg;
	msg = createTrackerFile(read_msg);
	cout << "Sending create tracker response..." << endl;
	if((write(sock_child, msg.c_str(), 100)) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(0);
	}
}

string createTrackerFile(char* read_msg) {
	TrackerFile tf = parseCreateTrackerMsg(read_msg);
	FILE *fp;
	string err = "<createtracker fail>";

	fp = fopen((trackerFilePath + "/" + tf.filename + ".track").c_str(), "r");

	if(fp) {
		for(int i = 0; i < trackerFiles.size(); i++) {
			if(trackerFiles[i].filename == tf.filename) {
				for(int j = 0; j < trackerFiles[i].peerlist.size(); i++) {
					if(trackerFiles[i].peerlist[j].ip == tf.peerlist[0].ip) {
						return "<createtracker ferr>";
					}
				}
			}
		}		
		
		fp = fopen((trackerFilePath + "/" + tf.filename + ".track").c_str(), "a");
		if(fputs((tf.peerlist[0].ip.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].port.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].start_byte.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].end_byte.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].timestamp.c_str()), fp) == EOF) { return err;}
	} else {
		if(!writeTrackerFile(tf)) {
			return "<createtracker fail>";
		}
	}	

	fclose(fp);
	trackerFiles.push_back(tf);

	return "<createtracker succ>";
}

TrackerFile parseCreateTrackerMsg(char* read_msg) {
	char* msg = read_msg;
	char time[100];
	TrackerFile tf;
	PeerInfo pi;

	strtok(msg, " ");
	tf.filename = strtok(NULL, " ");
	tf.filesize = strtok(NULL, " ");
	tf.description = strtok(NULL, " ");
	tf.md5 = strtok(NULL, " ");
	pi.ip = strtok(NULL, " ");
	pi.port = strtok(NULL, " ");
	pi.start_byte = "0";
	pi.end_byte = tf.filesize;
	sprintf(time, "%.f", difftime(timer, mktime(&newyear))); 
	pi.timestamp = time;
	tf.peerlist.push_back(pi);

	return tf;
}

void handle_list_req(int sock_child) {
	string msg = "<REP LIST ";
	stringstream ss;
	ss << trackerFiles.size();
	msg = msg + ss.str();
	ss.flush();
	msg = msg + ">\n";

	for(int i = 0; i < trackerFiles.size(); i++) {
		msg = msg + "<";
		ss << (i+1);
		msg += ss.str();
		ss.flush();
		msg += " ";
		msg += trackerFiles[i].filename;
		msg += " ";
		msg += trackerFiles[i].filesize;
		msg += " ";
		msg += trackerFiles[i].description;
		msg += " ";
		msg += trackerFiles[i].md5;
		msg += " ";
		for(int j = 0; j < trackerFiles[i].peerlist.size(); j++) {			
			msg += trackerFiles[i].peerlist[j].ip;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].port;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].start_byte;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].end_byte;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].timestamp;
			msg += ">\n";
		}
	}

	msg = msg + "<REP LIST END>\n";

	cout << "Sending list response..." << endl;
	if(write(sock_child, msg.c_str(), 1000) < 0) {
		printf("Send_request failure\n"); exit(0);
	}
}

void handle_get_req(int sock_child, char* read_msg) {
	string filename = parseGetRequest(read_msg);
	TrackerFile tf;
	char sendBuf[MAX_SEND_LENGTH];
	char filePathBuf[100];
	int fileBlockSize;

	for(int i = 0; i < trackerFiles.size(); i++) {
		if(trackerFiles[i].filename == filename) {
			tf = trackerFiles[i];
		}
	}

	strcpy(filePathBuf, trackerFilePath.c_str());
	strcat(filePathBuf, filename.c_str());
	strcat(filePathBuf, ".track");
	FILE *fs = fopen(filePathBuf, "rb");
	if(fs == 0) {
		cout << "error opening file" << endl;
	}

	strcpy(sendBuf, "<REP GET BEGIN>\n");
	if(send(sock_child, sendBuf, strlen(sendBuf), 0) < 0) {
		cout << "Error sending GET response header" << endl;
	}
	bzero(sendBuf, MAX_SEND_LENGTH);
	cout << "Sending tracker file to peer..." << endl;
	while((fileBlockSize = fread(sendBuf, sizeof(char), MAX_SEND_LENGTH, fs))) {
		if(send(sock_child, sendBuf, fileBlockSize, 0) < 0) {
			cout << "Error sending tracker file" << endl;
		}
		bzero(sendBuf, MAX_SEND_LENGTH);
	}
	strcpy(sendBuf, "\n<REP GET END ");
	strcat(sendBuf, tf.md5.c_str());
	strcat(sendBuf, ">");

	if(send(sock_child, sendBuf, strlen(sendBuf), 0) < 0) {
		cout << "Error sending GET response footer" << endl;
	}

	fclose(fs);	
}

string parseGetRequest(char* read_msg) {
	char* msg = read_msg;

	strtok(msg, " ");	
	return strtok(NULL, " ");
}

void handle_download(int sock_child, char* read_msg) {
	strtok(read_msg, " ");
	string filename = strtok(NULL, " ");
	int sendBuf[MAX_SEND_LENGTH];
	char filePathBuf[100];
	int fileBlockSize;
	int bytes_sent;

	strcpy(filePathBuf, sharedFilePath.c_str());
	strcat(filePathBuf, filename.c_str());
	FILE *fs = fopen(filePathBuf, "r+b");
	if(fs == NULL) {
		cout << "error opening file" << endl; exit(1);
	}

	bzero(sendBuf, MAX_SEND_LENGTH);
	fseek(fs, 0, SEEK_END);
	cout << "Sending " << ftell(fs) << " bytes..." << endl;
	rewind(fs);

	while((fileBlockSize = fread(sendBuf, sizeof(int), MAX_SEND_LENGTH, fs))) {
		if((bytes_sent = send(sock_child, sendBuf, fileBlockSize, 0)) < 0) {
			cout << "Error sending tracker file" << endl;
		}

		bzero(sendBuf, MAX_SEND_LENGTH);
	}
	fclose(fs);
}

void handle_updatetracker_req(int sock_child, char* read_msg) {
	string msg = updateTrackerFile(read_msg);
	cout << "Sending response: " << msg << endl;
	if((write(sock_child, msg.c_str(), 100)) < 0){
		printf("Send_request  failure\n"); exit(0);
	}
}

bool writeTrackerFile(TrackerFile tf) {
	FILE *fp;
	fp = fopen((trackerFilePath + "/" + tf.filename + ".track").c_str(), "w");
		if(fputs((tf.filename.c_str()), fp) == EOF) { return false;}
		fputs("\n", fp);
		if(fputs((tf.filesize.c_str()), fp) == EOF) { return false;}
		fputs("\n", fp);
		if(fputs((tf.description.c_str()), fp) == EOF) { return false;}
		fputs("\n", fp);
		if(fputs((tf.md5.c_str()), fp) == EOF) { return false;}
		fputs("\n", fp);
		for(int i = 0; i < tf.peerlist.size(); i++) {
			if(fputs((tf.peerlist[i].ip.c_str()), fp) == EOF) { return false;}
			fputs(":", fp);
			if(fputs((tf.peerlist[i].port.c_str()), fp) == EOF) { return false;}
			fputs(":", fp);
			if(fputs(tf.peerlist[i].start_byte.c_str(), fp) == EOF) { return false;}
			fputs(":", fp);
			if(fputs(tf.filesize.c_str(), fp) == EOF) { return false;}
			fputs(":", fp);
			if(fputs(tf.peerlist[i].timestamp.c_str(), fp) == EOF) { return false;}
			cout << tf.peerlist[i].timestamp << endl;
		}

	fclose(fp);

	return true;
}

string updateTrackerFile(char* read_msg) {	
	char buff[100];
	strcpy(buff, read_msg);
	int idx = parseUpdateTrackerMsg(read_msg);
	string result = "<updatetracker ";
	strtok(buff, " ");
	result += strtok(NULL, " ");

	if(idx != -1) {
		if(writeTrackerFile(trackerFiles[idx])) {
			result += " succ>";
			return result;
		} else {
			result += " fail>";
			return result;
		}
	}

	result += " ferr>";
	return result;
}

int parseUpdateTrackerMsg(char* read_msg) {
	char* msg = read_msg;
	char timeString[100];
	string filename;
	PeerInfo pi;

	strtok(msg, " ");
	filename = strtok(NULL, " ");
	for(int i = 0; i < trackerFiles.size(); i++) {
		if(trackerFiles[i].filename == filename) {
			pi.start_byte = strtok(NULL, " ");
			pi.end_byte = strtok(NULL, " ");
			pi.ip = strtok(NULL, " ");
			for(int j = 0; j < trackerFiles[i].peerlist.size(); j++) {
				cout << pi.ip << " and " << trackerFiles[i].peerlist[j].ip << endl;
				if(pi.ip == trackerFiles[i].peerlist[j].ip) {
					trackerFiles[i].peerlist[j].start_byte = pi.start_byte;
					trackerFiles[i].peerlist[j].end_byte = pi.end_byte;
					trackerFiles[i].peerlist[j].ip = pi.ip;
					trackerFiles[i].peerlist[j].port = strtok(NULL, " ");
					time(&timer);
					sprintf(timeString, "%.f", difftime(timer, mktime(&newyear))); 
					trackerFiles[i].peerlist[j].timestamp = timeString;	
					return i;
				} else {
					return -1;
				}

			}			
		}
	}
}
