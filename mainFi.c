/* header files */
#include	<stdio.h>
#include	<stdlib.h>
#include	<netdb.h>	/* getservbyname(), gethostbyname() */
#include	<errno.h>	/* for definition of errno */
#include        <string.h>
#include 	<fcntl.h>
#include	<termios.h>
#include	<time.h>
#include <unistd.h>

/* define macros*/
#define MAXBUF	1024
#define STDIN_FILENO	0
#define STDOUT_FILENO	1

/* define FTP reply code */
#define USERNAME	220
#define PASSWORD	331
#define LOGIN		230
#define PATHNAME	257
#define CLOSEDATA	226
#define ACTIONOK	250
#define PASSIVE         227
#define RENAME		350
#define ACTIVE		200

/* DefinE global variables */
char	*host;		/* hostname or dotted-decimal string */
char	*port;
char	*rbuf, *rbuf1;		/* pointer that is malloc'ed */
char	*wbuf, *wbuf1;		/* pointer that is malloc'ed */
struct sockaddr_in	servaddr;
struct termios before;
struct termios after;

int	cliopen(char *host, char *port);
void	strtosrv(char *str, char *host, char *port);
void	cmd_tcp(int sockfd);
void	ftp_list(int sockfd);
int	ftp_get(int sck, char *pDownloadFileName_s,int rate,int type, int flength);
int	ftp_put (int sck, char *pUploadFileName_s,int rate, int type);
int 	clilisten(char *host, char *port);

int
main(int argc, char *argv[])
{
	int	fd;
	
	host = (char*)malloc(sizeof(char)*MAXBUF);
        port = (char*)malloc(sizeof(char)*MAXBUF);
	
	if (0 != argc-2)
	{
		printf("%s\n","missing <hostname>");
		exit(0);
	}

	host = argv[1];
	port = "21";

	/*****************************************************************
	//1. code here: Allocate the read and write buffers before open().
	*****************************************************************/
	rbuf = (char*)malloc(sizeof(char)*MAXBUF);
	rbuf1 = (char*)malloc(sizeof(char)*MAXBUF);
	wbuf = (char*)malloc(sizeof(char)*MAXBUF);
	wbuf1 = (char*)malloc(sizeof(char)*MAXBUF);
	
	fd = cliopen(host, port);

	cmd_tcp(fd);

	exit(0);
}

/* Establish a TCP connection from client to server */
int
cliopen(char *host, char *port)
{
	/*************************************************************
	//2. code here 
	*************************************************************/
	int sock; /* Socket descriptor */
	unsigned short echoServPort = atoi(port); /* Echo server port */
	char *servIP = host; /* IP address of server */
	
	/* Create a TCP socket */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		printf("socket() failed.\n");

	/* Construct the server address structure */
	memset(&servaddr, 0, sizeof(servaddr));/*Zero out structure*/
	servaddr.sin_family = AF_INET; /* Internet addr family */
	servaddr.sin_addr.s_addr = inet_addr(servIP);/*Server IP address*/
	servaddr.sin_port = htons(echoServPort); /* Server port */

	if((connect(sock,(struct sockaddr *)&servaddr,sizeof(servaddr)))<0)
		printf("connect() failed\n");
	else
	{
		printf("Connect to server:%s\n",servIP);
		return sock;
	}
}

/*Active mode listen*/
int
clilisten(char *host,char *port)
{
	int sock; /* Socket descriptor */
        unsigned short echoServPort = atoi(port); /* Echo server port */
        char *servIP = host; /* IP address of server */

        /* Create a TCP socket */
        if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                printf("socket() failed.\n");

        /* Construct the server address structure */
        memset(&servaddr, 0, sizeof(servaddr));/*Zero out structure*/
        servaddr.sin_family = AF_INET; /* Internet addr family */
        servaddr.sin_addr.s_addr = inet_addr(servIP);/*Server IP address*/
        servaddr.sin_port = htons(echoServPort); /* Server port */
	
	if ((bind(sock, (struct sockaddr *)&servaddr,sizeof(servaddr))) < 0)
		printf("bind() failed.\n");

	if((listen(sock,10))<0)
		printf("listen() failed.\n");
	else
	{	
		printf("listen successful.\n");
		return sock;
	}
}

/*
   Compute server's port by a pair of integers and store it in char *port
   Get server's IP address and store it in char *host
*/
void
strtosrv(char *str, char *host, char *port)
{
	/*************************************************************
	//3. code here
	*************************************************************/
	int params[7];
	
	sscanf(str, "%d Entering Passive Mode (%d,%d,%d,%d,%d,%d)",&params[0],&params[1],&params[2],&params[3],&params[4],&params[5],&params[6]);
	
	sprintf(host,"%d.%d.%d.%d",params[1],params[2],params[3],params[4]);
	sprintf(port,"%d",params[5]*256+params[6]);
}

/* Read and write as command connection */
void
cmd_tcp(int sockfd)
{
	int		maxfdp1, nread, nwrite, fd, flength, replycode;
	int 		flag; //0 ls 1 get 2 put 3 delete 4 cd/rmdir 5 rename
	int		type=0;//0 ascii 1 binary
	int		rate=0;
	char		host[16];
	char		port[6];
	char 		*filename;
        filename = (char *)malloc(sizeof(char)*MAXBUF);
	fd_set		rset;

	FD_ZERO(&rset);
	maxfdp1 = sockfd + 1;	/* check descriptors [0..sockfd] */

	for ( ; ; )
	{	
		FD_SET(STDIN_FILENO, &rset);//standard input 
		FD_SET(sockfd, &rset);


		if (select(maxfdp1, &rset, NULL, NULL, NULL) < 0)
			printf("select error\n");
			
		/* data to read on stdin */
		if (FD_ISSET(STDIN_FILENO, &rset)) {
			if ( (nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0)
				printf("read error from stdin\n");
			rbuf[nread] = '\0';
			nwrite = nread+5;

			/* send username */
			if (replycode == USERNAME) {
				sprintf(wbuf, "USER %s", rbuf);
				if (write(sockfd, wbuf, nwrite) != nwrite)
					printf("write error\n");
			}

			 /*************************************************************
			 //4. code here: send password
			 *************************************************************/
			/*send password*/
			if (replycode == PASSWORD) {
				tcsetattr(STDIN_FILENO,TCSANOW,&before);
                                sprintf(wbuf, "PASS %s", rbuf);
                                if (write(sockfd, wbuf, nwrite) != nwrite)
                                        printf("write error\n");
                        }
			
			 /* send command */
			if (replycode==LOGIN || replycode==CLOSEDATA || replycode==PATHNAME || replycode==ACTIONOK)
			{
				/*passive - passvice mode*/
				if (strncmp(rbuf, "passive", 7) == 0)
				{
                                        sprintf(wbuf, "%s", "PASV\n");
                                        write(sockfd, wbuf, 5);
					continue;
				}
				
				/* pwd -  print working directory */
                                if (strncmp(rbuf, "pwd", 3) == 0) {
                                        sprintf(wbuf, "%s", "PWD\n");
                                        write(sockfd, wbuf, 4);
                                        continue;
                                }				
				
				/*************************************************************
                                // 5. code here: cd - change working directory/
                                *************************************************************/
                                if (strncmp(rbuf, "cd", 2) == 0)
                                {
                                		char *path;
										path = (char *)malloc(sizeof(char)*MAXBUF);						
										sscanf(rbuf,"cd %s",path);
										
										if(strlen(path) == 0){
											if (write(STDOUT_FILENO, "missing <pathname>\n", 19) != 19)
												printf("write error to stdout\n");
										}
										else{										
											sprintf(wbuf, "CWD %s\n",path);
	                                        write(sockfd, wbuf, strlen(path) + 5);
										}
										flag = 4;
          								continue;
                                }
				
				/*make new director*/				
				if(strncmp(rbuf,"mkdir",5)==0)
				{
					char *path;
					path = (char *)malloc(sizeof(char)*MAXBUF);

					sscanf(rbuf,"mkdir %s",path);
					if(strlen(path) == 0){
							if (write(STDOUT_FILENO, "missing <pathname>\n", 19) != 19)
							printf("write error to stdout\n");
					}
					else{
					sprintf(wbuf,"MKD %s\n",path);
					 write(sockfd, wbuf, strlen(path) + 5);						
					}
					flag = 6;
					continue;

				}
				
				/*delete a file*/
                                if(strncmp(rbuf,"delete",5)==0)
                                {
                                        char *path;
                                        path = (char *)malloc(sizeof(char)*MAXBUF);
                                        sscanf(rbuf,"delete %s",path);
										if(strlen(path) == 0){
											if (write(STDOUT_FILENO, "missing <filename>\n", 19) != 19)
											printf("write error to stdout\n");
										}
										else{
	                                        sprintf(wbuf,"DELE %s\n",path);
	                                        write(sockfd, wbuf, strlen(path) + 6);
										}
										flag = 3;
                                        continue;
                                }
				
				/*delete a folder*/
                                if(strncmp(rbuf,"rmdir",5)==0)
                                {
                                        char *path;
                                        path = (char *)malloc(sizeof(char)*MAXBUF);
                                        sscanf(rbuf,"rmdir %s",path);
                                        if(strlen(path) == 0){
											if (write(STDOUT_FILENO, "missing <pathname>\n", 19) != 19)
											printf("write error to stdout\n");
										}
										else{
										sprintf(wbuf,"RMD %s\n",path);
                                        write(sockfd, wbuf, strlen(path) + 5);	
										}
										flag = 4;
                                        continue;
                                }

				/*rename file*/
                                if(strncmp(rbuf,"rename",6)==0)
                                {
                                        char *path;
                                        path = (char *)malloc(sizeof(char)*MAXBUF);
										char *newpath;
    									newpath = (char*)malloc(sizeof(char)*MAXBUF);
    									sscanf(rbuf,"rename %s %s",path,newpath);
										if(strlen(path) == 0 || strlen(newpath) == 0){
									    	if (write(STDOUT_FILENO, "missing <filename> <newfilename>\n", 34) != 34)
											printf("write error to stdout\n");									    	
									    }
										else{
										sprintf(wbuf,"RNFR %s\n",path);
                                        write(sockfd, wbuf, strlen(path) + 6);			
										sprintf(wbuf,"RNTO %s\n",newpath);
										write(sockfd, wbuf, strlen(newpath) + 6);	
										}       
										flag = 5;                                 
                                        continue;
                                }
				
				/*ascii*/
				if (strncmp(rbuf, "ascii", 5) == 0)
                                {
                                        sprintf(wbuf, "%s\n","TYPE A");
					type = 0;
                                        write(sockfd, wbuf, 7);
                                        continue;
                                }
				
				/*binary*/
                                if (strncmp(rbuf, "binary", 6) == 0)
                                {       
                                        sprintf(wbuf, "%s\n","TYPE I");
                                	type = 1;
                                        write(sockfd, wbuf, 7);
                                        continue;
                                }    
				
				/*Active*/
				if(strncmp(rbuf, "port", 4)==0)
				{
					int params[6];
					
				        sscanf(rbuf, "port %d,%d,%d,%d,%d,%d",&params[0],&params[1],&params[2],&params[3],&params[4],&params[5]);
				       
						sprintf(wbuf, "PORT %d,%d,%d,%d,%d,%d\n",params[0],params[1],params[2],params[3],params[4],params[5]);
				        sprintf(port,"%d",params[4]*256+params[5]);	
					write(sockfd,wbuf,strlen(wbuf));	
						
						continue;
				}
			
				/*Quit*/
				if (strncmp(rbuf, "quit",4)==0)
				{
					sprintf(wbuf,"%s","QUIT\n");
					write(sockfd,wbuf,5);
					continue;
				}

				write(sockfd, rbuf, nread);

			}
			
			if(replycode == PASSIVE||replycode == ACTIVE)
			{
				
				/* ls - list files and directories*/
                               	if (strncmp(rbuf, "ls", 2) == 0) 
				{
                                       	sprintf(wbuf1, "%s", "LIST -al\n");
	                                nwrite = 9;
					write(sockfd, wbuf1, nwrite);
					nwrite = 0;
					flag = 0;
        	                        continue;
                	        }

				if(strncmp(rbuf, "limit",5)==0)
				{
					sscanf(rbuf,"limit %d",&rate);
					if(rate == 0){
						if (write(STDOUT_FILENO, "missing <rate>\n", 15) != 15)
								printf("write error to stdout\n");					
					}
					continue;
				}
	
        	                /*************************************************************
                	        // 5. code here: cd - change working directory/
                                 *************************************************************/
			       	if (strncmp(rbuf, "cd", 2) == 0)
                                {
                                		char *path;
										path = (char *)malloc(sizeof(char)*MAXBUF);						
										sscanf(rbuf,"cd %s",path);
										
										if(strlen(path) == 0){
											if (write(STDOUT_FILENO, "missing <pathname>\n", 19) != 19)
												printf("write error to stdout\n");
										}
										else{										
											sprintf(wbuf, "CWD %s\n",path);
	                                        write(sockfd, wbuf, strlen(path) + 5);
										}
										flag = 4;
          								continue;
                                }


                                /* pwd -  print working directory */
        	                if (strncmp(rbuf, "pwd", 3) == 0) 
				{
					sprintf(wbuf, "%s", "PWD\n");
                       		        write(sockfd, wbuf, 4);
                       	        	continue;
			        }
				
				/*make new director*/           
                                if(strncmp(rbuf,"mkdir",5)==0)
				{
					char *path;
					path = (char *)malloc(sizeof(char)*MAXBUF);

					sscanf(rbuf,"mkdir %s",path);
					if(strlen(path) == 0){
							if (write(STDOUT_FILENO, "missing <pathname>\n", 19) != 19)
							printf("write error to stdout\n");
					}
					else{
					sprintf(wbuf,"MKD %s\n",path);
					 write(sockfd, wbuf, strlen(path) + 5);						
					}
					flag = 6; 
					continue;

				}
			
				/*delete a file*/
                                if(strncmp(rbuf,"delete",5)==0)
                                {
                                        char *path;
                                        path = (char *)malloc(sizeof(char)*MAXBUF);
                                        sscanf(rbuf,"delete %s",path);
										if(strlen(path) == 0){
											if (write(STDOUT_FILENO, "missing <filename>\n", 19) != 19)
											printf("write error to stdout\n");
										}
										else{
	                                        sprintf(wbuf,"DELE %s\n",path);
	                                        write(sockfd, wbuf, strlen(path) + 6);
										}
										flag = 3;
                                        continue;
                                }

				/*delete a folder*/
                                if(strncmp(rbuf,"rmdir",5)==0)
                                {
                                        char *path;
                                        path = (char *)malloc(sizeof(char)*MAXBUF);
                                        sscanf(rbuf,"rmdir %s",path);
                                        if(strlen(path) == 0){
											if (write(STDOUT_FILENO, "missing <pathname>\n", 19) != 19)
											printf("write error to stdout\n");
										}
										else{
										sprintf(wbuf,"RMD %s\n",path);
                                        write(sockfd, wbuf, strlen(path) + 5);	
										}
										flag = 4;
                                        continue;
                                }

	                        /*************************************************************
        	                // 6. code here: quit - quit from ftp server
                	        *************************************************************/
                                if (strncmp(rbuf, "quit",4)==0)
                                {
                                        sprintf(wbuf,"%s","QUIT\n");
                                        write(sockfd,wbuf,5);
                                        continue;
                                }
				
				/*rename file*/
                                if(strncmp(rbuf,"rename",6)==0)
                                {
                                        char *path;
                                        path = (char *)malloc(sizeof(char)*MAXBUF);
										char *newpath;
    									newpath = (char*)malloc(sizeof(char)*MAXBUF);
    									sscanf(rbuf,"rename %s %s",path,newpath);
										if(strlen(path) == 0 || strlen(newpath) == 0){
									    	if (write(STDOUT_FILENO, "missing <filename> <newfilename>\n", 34) != 34)
											printf("write error to stdout\n");									    	
									    }
										else{
										sprintf(wbuf,"RNFR %s\n",path);
                                        write(sockfd, wbuf, strlen(path) + 6);			
										sprintf(wbuf,"RNTO %s\n",newpath);
										write(sockfd, wbuf, strlen(newpath) + 6);	
										}     
										flag = 5;                                   
                                        continue;
                                }
				
				/*ascii*/
                                if (strncmp(rbuf, "ascii", 5) == 0)
                                {       
                                        sprintf(wbuf, "%s\n","TYPE A");
 					type = 0;                               
                                        write(sockfd, wbuf, 7);
                                        continue;
                                }       
                                
                                /*binary*/
                                if (strncmp(rbuf, "binary", 6) == 0)
                                {
                                        sprintf(wbuf, "%s\n","TYPE I");
                                	type = 1;
                                        write(sockfd, wbuf, 7);
                                        continue;
                                }

	                        /*************************************************************
        	                // 7. code here: get - get file from ftp server
                                *************************************************************/
		
                    if (strncmp(rbuf, "getfile", 7) == 0){
						sscanf(rbuf,"getfile %s",filename);
						if(strlen(filename) == 0){
							if (write(STDOUT_FILENO, "missing <filename>\n", 19) != 19)
								printf("write error to stdout\n");
						}
						else{
							if(access(filename,0)==0){
								for(;;){
									if (write(STDOUT_FILENO, "File exist, continue download? Y/N\n", 36) != 36)
										printf("write error to stdout\n");
									if ( (nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0)
										printf("read error from stdin\n");
									if(strncmp(rbuf,"Y",1) == 0 || strncmp(rbuf,"y",1) == 0){
									int i,filefd,length,j;
									char *content;
									content = (char*)malloc(sizeof(char)*MAXBUF);
			
									filefd = open(filename,O_RDWR);
									length = lseek(filefd,0,SEEK_END);
									j = length;						        
									lseek(filefd,0,SEEK_SET);
			
			        					read(filefd,content,length);
			        					content[length] = '\0';
									
									if(type == 0)
									{
										for(i=0;i<j;i++)
											if(content[i]=='\n')
												length++;
									}
			
									sprintf(wbuf1,"REST %d\n",length);
									write(sockfd,wbuf1,strlen(wbuf1));
									recv(sockfd, rbuf1, MAXBUF, 0);
									close(filefd);
									break;									
									}
									else if(strncmp(rbuf,"N",1) == 0 || strncmp(rbuf,"n",1) == 0)
										break;									
									else
										continue;
								}
								if(strncmp(rbuf,"N",1) == 0 || strncmp(rbuf,"n",1) == 0){
									if (write(STDOUT_FILENO, "Download cancelled\n", 19) != 19)
										printf("write error to stdout\n");
									continue;									
								}

							}
							sprintf(wbuf,"SIZE %s\n",filename);
							write(sockfd,wbuf,strlen(filename)+6);
							recv(sockfd,rbuf,MAXBUF,0);
							sscanf(rbuf, "213 %d", &flength);
							
							flag = 1;
	                        sprintf(wbuf1, "RETR %s\n", filename);
	                        write(sockfd, wbuf1, strlen(filename) + 6);
	                        nwrite = 0;	
						}
						continue;
                    }

	                        /***********************************************************
       	                        // 8. code here: put -  put file upto ftp server
               	                *************************************************************/
				if (strncmp(rbuf, "putfile", 7) == 0){
                    sscanf(rbuf,"putfile %s",filename);
                    if(strlen(filename) == 0){
							if (write(STDOUT_FILENO, "missing <filename>\n", 19) != 19)
								printf("write error to stdout\n");
					}
					else{
						if((open(filename,O_RDWR))<0)
						{
							write(STDOUT_FILENO,"No File\n",8);
						}
						else
						{
	                      	sprintf(wbuf1, "STOR %s\n", filename);
	                        write(sockfd, wbuf1, strlen(filename) + 6);
	                        nwrite = 0;
	                        flag = 2;
						}						
					}

                    continue;
               }

				write(sockfd, rbuf, nread);
			}
		}
		/* data to read from socket */
		if (FD_ISSET(sockfd, &rset)) {
			if ( (nread = recv(sockfd, rbuf, MAXBUF, 0)) < 0)
				printf("recv error\n");
			else if (nread == 0)
				break;

			/* set replycode and wait for user's input */
			if (strncmp(rbuf, "220", 3)==0 || strncmp(rbuf, "530", 3)==0){
				rbuf[nread] = '\0';
				strcat(rbuf,  "your name: ");
				nread += 12;
				replycode = USERNAME;
			}
			
			/*************************************************************
			// 9. code here: handle other response coming from server
			*************************************************************/
			if (strncmp(rbuf, "331", 3)==0){
				rbuf[nread] = '\0';
                                strcat(rbuf,  "your password: ");
				tcgetattr(0, &before); 
                                after = before;
                                after.c_lflag &= ~ECHO;
                   		tcsetattr(STDIN_FILENO,TCSANOW,&after);
			        nread += 16;
                                replycode = PASSWORD;
                        }
				
			if(strncmp(rbuf, "230", 3)==0)
			{	
				rbuf[nread] = '\0';
				char help[] = "-----------------------------help---------------------------\npassive-----------------------------------------Passive mode\nport + IP + PORT---------------------------------Active mode\nls---------------------------------------------List all file\nascii---------------------------------------------ASCII mode\nbinary-------------------------------------------Binary mode\npwd-----------------------------------View current directory\ncd + folder_name ---------------------------Change directory\nquit----------------------------------------------------Quit\nmkdir + folder_name----------------------Create new director\nrmdir + folder_name------------------------Remove a director\ndelete + file_name-----------------------------Delete a file\nrename + old + new--------------------------Rename file name\ngetfile + file_name----------------------------Download File\nputfile + file_name------------------------------Update File\nlimit + Integer-----------------------------------Limit Rate\n\n";
				strcat(rbuf, help);
				nread += strlen(help);
				replycode = LOGIN;	
			}

			/* open data connection*/
			if (strncmp(rbuf, "227", 3) == 0) {
				rbuf[nread] = '\0';
				strtosrv(rbuf, host, port);
				fd = cliopen(host, port);
				replycode = PASSIVE;
			}
			
			if (strncmp(rbuf, "200 P", 5) == 0)
			{
				rbuf[nread] = '\0';
				sprintf(host,"%s","10.0.2.15");

				fd = clilisten(host,port);			
				replycode = ACTIVE;
			}
			
			if (strncmp(rbuf,"550",3) == 0)
			{
				char* prompt;
				prompt = (char *)malloc(sizeof(char)*MAXBUF);
				if(flag == 1 || flag == 3){
					prompt = "File not found\n";
					strcpy(rbuf,prompt);
					nread = strlen(prompt);
					rbuf[nread] = '\0';					
				}
				if(flag == 4){
					prompt = "Directory not found\n";
					strcpy(rbuf,prompt);
					nread = strlen(prompt);
					rbuf[nread] = '\0';						
				}
				if(flag == 5){
					prompt = "Directory/File not found\n";
					strcpy(rbuf,prompt);
					nread = strlen(prompt);
					rbuf[nread] = '\0';						
				}
				if(flag == 6){
					prompt = "Directory exist\n";
					strcpy(rbuf,prompt);
					nread = strlen(prompt);
					rbuf[nread] = '\0';						
				}
			}

			if (strncmp(rbuf,"553",3) == 0)
			{
				if(flag == 2)
					printf("File Exist\n");
			}
			
			/* start data transfer */
			if (write(STDOUT_FILENO, rbuf, nread) != nread)
				printf("write error to stdout\n");

			/*list*/
			if(strncmp(rbuf,"150",3) == 0)
			{
				rbuf[nread] = '\0';
				if(replycode == ACTIVE)
				{
					int datafd,cliAddrLen;
					struct sockaddr_in echoClntAddr;
					cliAddrLen = sizeof(echoClntAddr);

					if((datafd = accept(fd,(struct sockaddr *)&echoClntAddr,&cliAddrLen))<0)
						printf("accept() failed\n");
					 
					if(flag==0)
                                        	ftp_list(datafd);
                               		if(flag==1)
                                        	ftp_get(datafd,filename,rate,type,flength);
                                	if(flag==2)
                                        	ftp_put(datafd,filename,rate,type);
					
					close(datafd);
				}
				else
				{
					if(flag==0)
						ftp_list(fd);
					if(flag==1)
						ftp_get(fd,filename,rate,type,flength);			
					if(flag==2)
						ftp_put(fd,filename,rate,type);
				}
	
				replycode = CLOSEDATA;
			}
		}
	}		
	
	if (close(sockfd) < 0)
		printf("close error\n");
}


/* Read and write as data transfer connection */
void
ftp_list(int sockfd)
{
	int		nread;

	for ( ; ; )
	{
		/* data to read from socket */
		if ( (nread = recv(sockfd, rbuf1, MAXBUF, 0)) < 0)
			printf("recv error\n");
		else if (nread == 0)
			break;

		if (write(STDOUT_FILENO, rbuf1, nread) != nread)
			printf("send error to stdout\n");
	}

	if (close(sockfd) < 0)
		printf("close error\n");
}

/* download file from ftp server */
int	
ftp_get(int sck, char *pDownloadFileName_s,int rate,int type,int flength)
{
	/*************************************************************
	// 10. code here
	*************************************************************/
        int	i = 1,j,nread;
	int		filefd;
	int length = 0;
	char		*content;
	content = (char*)malloc(sizeof(char)*MAXBUF);

	if(access(pDownloadFileName_s,0)==-1)	
	{
		filefd = creat(pDownloadFileName_s,0666);
		if(type==0)
		{
			for(;;)
			{
				if ( (nread = recv(sck, rbuf1, 1, 0)) < 0)
        	                	printf("recv error\n");
	                	else if (nread == 0)
                	        	break;
			
				if(rbuf1[0]=='\r'){
					length++;
					continue;					
				}

				else
					write(filefd,rbuf1,1);
				if(rate>0)
					usleep(1000000/rate);
				length++;
				if(length *20 >= flength* i){
					for(j=1;j<=20;j++){
						if(j<=i)
							printf("*");
						else 		
							printf("_");

					}
						printf("\n");					
						printf("\033[1A"); 
        				printf("\033[K");
					i++;	
				}
			}
			write(filefd,'\0',1);
		}
		else
		{
			for(;;)
                        {
                                if ( (nread = recv(sck, rbuf1, 1, 0)) < 0)
                                        printf("recv error\n");
                                else if (nread == 0)
                                        break;

                                write(filefd,rbuf1,1);

                                if(rate>0)
                                	usleep(1000000/rate);
								length++;
								if(length *20 >= flength* i){
									for(j=1;j<=20;j++){
										if(j<=i)
											printf("*");
										else 		
											printf("_");
				
									}
										printf("\n");					
										printf("\033[1A"); 
				        				printf("\033[K");
									i++;	
								}                             
                        }
		}		
	}
       	else
	{
		filefd = open(pDownloadFileName_s,O_RDWR);
		length = lseek(filefd,0,SEEK_END);
		if(type==0)
		{
			for(;;)
        	        {
                		if ( (nread = recv(sck, rbuf1, 1, 0)) < 0)
        		                printf("recv error\n");
                	        else if (nread == 0) 
                        	       break;
				
				if(rbuf1[0]=='\r'){ 
					length++; 
                	continue;
    			} 
				else
					write(filefd,rbuf1,1);
				if(rate>0)
                	usleep(1000000/rate);
								length++;
								if(length *20 >= flength* i){
									for(j=1;j<=20;j++){
										if(j<=i)
											printf("*");
										else 		
											printf("_");
				
									}
										printf("\n");					
										printf("\033[1A"); 
				        				printf("\033[K");
									i++;	
								}   
			}
		}
		else
		{
                        for(;;)
                        {
                                if ( (nread = recv(sck, rbuf1, 1, 0)) < 0)
                                        printf("recv error\n");
                                else if (nread == 0)
                                       break;
                                if(rate>0)
               				 	usleep(1000000/rate);

	      	                write(filefd,rbuf1,1);
	      	                								length++;
								if(length *20 >= flength* i){
									for(j=1;j<=20;j++){
										if(j<=i)
											printf("*");
										else 		
											printf("_");
				
									}
										printf("\n");					
										printf("\033[1A"); 
				        				printf("\033[K");
									i++;	
								}  
			}
		}		
	}
	close(filefd);
	
	if (close(sck) < 0)
        	printf("close error\n");
}

/* upload file to ftp server */
int 
ftp_put (int sck, char *pUploadFileName_s, int rate, int type)
{
	/*************************************************************
	// 11. code here
	*************************************************************/
	int             nread;
        int             filefd;
	int		i;
	int		length=0;

	if((filefd = open(pUploadFileName_s,O_RDWR))<0)
		printf("No file\n");

	length = lseek(filefd,0,SEEK_END);

	lseek(filefd,0,SEEK_SET);
	
	if(type==0)
	{
		for(;;)
		{
			if((nread = read(filefd,rbuf1,1))==0)
				break;
			rbuf1[1] = '\0';		
			if(rbuf1[0]=='\n')
			{
				write(sck,"\r\n",2);
			}	
			else
				write(sck,rbuf1,1);

			if(rate>0)
                        	usleep(1000000/rate);

		}
	}
	else
	{	
		for(;;)
		{
			if((nread = read(filefd,rbuf1,MAXBUF))==0)
                                break;

                        write(sck,rbuf1,MAXBUF);

                        if(rate>0)
                        	usleep(1000000/rate);

		}	
	}
	
	close(filefd);
	
        if (close(sck) < 0)
                printf("close error\n");

}
