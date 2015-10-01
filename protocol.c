#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <math.h>
#include <regex.h>

#define Max_Packet_Length 13456

typedef struct print_data{
    char filename[100]; //filename
    off_t size; //size
    time_t mtime; //last modified
    char type[5]; //filetype
    unsigned char hash[MD5_DIGEST_LENGTH];
}print_data;

int tcp_server(int listenportno);
int tcp_client(char* ip, int connectportno);
int get_type(char *request);
void IndexGet_handler(char *request);
void FileDownload_handler(char *request);
int handleFileDownload(char *filename);
int handleCheckAll();
int udp_server(int listenportno);
int udp_client(char* ip, int connectportno);
void FileHash_handler(char *request);
int handleVerify(char *file);

int iter,i, hist_count = 0, error = -1;
struct print_data pdata[1024],hdata[1024];
char history[1024][1024] = {0};
char fileDownloadName[1024];
char response[10240],cresponse[10240];
int regex = 0;
char delim[] = " \n";



void main(int argc,char *argv[])
{
    char *list_port,*conn_port, *ip;
    pid_t pid;
    if(argc != 4)
    {
        printf("Plz give the correct arguments.\n");
        return;
    } 
    ip = argv[1];
    conn_port = argv[2];
    list_port= argv[3];
    pid = fork();
    if(pid == 0)
    {
        tcp_server(atoi(list_port));
    }
    else if(pid > 0)
    {        
       tcp_client(ip,atoi(conn_port));
    }
    
}

int tcp_server(int list_port)
{
    int listenfd = 0;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        perror("Unable to create socket");
        exit(0);
    }
    char read_buffer[1024], write_buffer[1024];
    struct sockaddr_in serv_addr; 
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(read_buffer, 0, sizeof(read_buffer)); 
    memset(write_buffer, 0, sizeof(write_buffer)); 
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(list_port); 
    
    int fileupload_flag = 0;
       
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Unable to bind");
        exit(0);
    }

    listen(listenfd, 10); 
    int connfd = 0;

    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
    int data;
    error = -1;
    data = read(connfd,read_buffer,sizeof(read_buffer));


    while( data > 0)
    {
        char *request = malloc(strlen(read_buffer)+1);
        strcpy(request,read_buffer);
        strcpy(history[hist_count],read_buffer);
        hist_count++;

        int type_request = get_type(request);
        printf("\nRequest Received : %s",request);
        response[0] = '\0';
        write_buffer[0] = '\0';
       
        printf("Enter Request Here :");
        fflush(stdout);
        if(type_request == -1)      //Error
        {
            error = 1;
            sprintf(response,"Invalid request\n");
        }
        else if(type_request == 1)      //Indexget
        {
            IndexGet_handler(request);
            //memset(wri, 0, sizeof(response));
        }
        /*else if(type_request == 2)      //FileHash
            FileHash_handler(request);
        else if(type_request == 3)      //FileDownload
            FileDownload_handler(request);
            */
        if(error == 1)
        {
            strcat(write_buffer,response);
            strcat(write_buffer,"~^~");
            write(connfd , write_buffer , strlen(write_buffer));
            error = -1;
            memset(write_buffer, 0, sizeof(write_buffer));
            memset(read_buffer, 0, sizeof(read_buffer)); 
            while((data = read(connfd,read_buffer,sizeof(read_buffer)))<=0);
            continue;
        }
        if(type_request == 1)
        {
            memset(response, 0, sizeof(response));
            memset(write_buffer,0, sizeof(write_buffer)); 
            //strcat(write_buffer,response);
            int a;
            //printf("I am i : %d\n",i);
            for(a = 0 ; a < iter ; a++)
            {
                sprintf(response, "%s| %d| %s| %s" , pdata[a].filename , (int)pdata[a].size , pdata[a].type , ctime(&pdata[a].mtime));
                strcat(write_buffer,response);
            }
            strcat(write_buffer,"~^~");
            write(connfd , write_buffer , strlen(write_buffer));
        }
        /*else if(type_request == 2)
        {
            memset(write_buffer, 0, sizeof(write_buffer)); 
            strcat(write_buffer,response);

            int b,c;
            for (b = 0 ; b < i ; b++)
            {
                sprintf(response, "%s |   ",hdata[b].filename);
                strcat(write_buffer,response);
                for (c = 0 ; c < MD5_DIGEST_LENGTH ; c++)
                {
                    sprintf(response, "%x",hdata[b].hash[c]);
                    strcat(write_buffer,response);
                }
                sprintf(response, "\t %s",ctime(&hdata[b].mtime));
                strcat(write_buffer,response);
            }
            strcat(write_buffer,"$^*");

            write(connfd , write_buffer , strlen(write_buffer));
        }
        else if(type_request == 3)
        {
            FILE* fp;
            fp = fopen(fileDownloadName,"rb");
            size_t bytes_read;
            while(!feof(fp))
            {
                bytes_read = fread(response, 1, 1024, fp);
                memcpy(write_buffer,response,bytes_read);
                write(connfd , write_buffer , bytes_read);
                memset(write_buffer, 0, sizeof(write_buffer));
                memset(response, 0, sizeof(response));
            }
            memcpy(write_buffer,"$^*",3);
            write(connfd , write_buffer , 3);
            memset(write_buffer, 0, sizeof(write_buffer));
            fclose(fp);
        }
        else if(type_request == 4)
        {
            printf("\nWould like to accept upload file : press 1 for yes else press 0\n");
            memset(write_buffer, 0,sizeof(write_buffer));
            fgets(write_buffer,sizeof(write_buffer),stdin);
            //scanf("%d",&fileupload_flag);
            fileupload_flag = 1;
            if(fileupload_flag == 1)
            {
                memcpy(writeBuff,"FileUpload Accept\n",18);
                write(connfd , writeBuff , 18);
                memset(writeBuff,0,18);
            }
            else
            {
                memcpy(writeBuff,"FileUpload Deny\n",16);
                write(connfd , writeBuff , 16);
                memset(writeBuff,0,16);
            }
        }

        if(fileupload_flag == 1)
        {   
            char copyrequest[1024];
            memset(copyrequest, 0,1024);
            memcpy(copyrequest,request,1024);
            char *size = strtok(copyrequest,"\n");
            size = strtok(NULL,"\n");
            long fsize = atol(size);
            char *request_data = NULL;
            const char delim[] = " \n";
            request_data = strtok(request,delim);
            request_data = strtok(NULL,delim);
            int f;
            int result;
            f = open(request_data, O_WRONLY | O_CREAT | O_EXCL, (mode_t)0600);
            if (f == -1) {
                perror("Error opening file for writing:");
                return 1;
            }
            result = lseek(f,fsize-1, SEEK_SET);
            result = write(f, "", 1);
            if (result < 0) {
                close(f);
                perror("Error opening file for writing:");
                return 1;
            }
            close(f);
            FILE *fp;
            fp = fopen(request_data,"wb");
            n = read(connfd, readBuff, sizeof(readBuff)-1);
            while(1)
            {
                readBuff[n] = 0;
                if(readBuff[n-1] == '*' && readBuff[n-3] == '$' && readBuff[n-2] == '^')
                {
                    readBuff[n-3] = 0;
                    fwrite(readBuff,1,n-3,fp);
                    fclose(fp);
                    memset(readBuff, 0,n-3);
                    break;
                }
                else
                {
                    fwrite(readBuff,1,n,fp);
                    memset(readBuff, 0,n);
                }
                n = read(connfd, readBuff, sizeof(readBuff)-1);
                if(n < 0)
                    break;
            }
            

        }
        */
        //memset(writeBuff, 0,1024);
        regex = 0;
        //fileupload_flag = 0;
        memset(read_buffer, 0, sizeof(read_buffer)); 
        memset(write_buffer, 0, sizeof(write_buffer));
        while((data = read(connfd,read_buffer,sizeof(read_buffer)))<=0);
    }
    close(connfd);
    wait(NULL);
}

int tcp_client(char *ip,int conn_port)
{
    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    int data = 0;
    if(sockfd < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 0;
    } 

    memset(&serv_addr, 0, sizeof(serv_addr)); 
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(conn_port); 

    char read_buffer[1024],write_buffer[1024];
    
    memset(read_buffer, 0,sizeof(read_buffer));
    memset(write_buffer, 0,sizeof(write_buffer));
    
    char FILE_NAME[1024];
        
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 0;
    } 

    while(1)
    {
        if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            continue;
        }
        else
        {
            printf("Client is Connected now : \n");
            break;
        }
    }

    int flag_download = 0 , flag_upload = 0;
    while(1)
    {

        printf("Your request here : ");
        flag_download = 0, flag_upload = 0;
        FILE *fp = NULL;
        int i = 0;        
        fgets(write_buffer,sizeof(write_buffer),stdin);
        char *filename;
        char copy[1024];
        
        strcpy(copy,write_buffer);
        filename = strtok(copy," \n");
        
        if(strcmp(filename,"quit") == 0)
            _exit(1);
        
        if(strcmp(filename,"FileDownload") == 0)
        {
            flag_download = 1;
            filename = strtok(NULL," \n");
            strcpy(FILE_NAME,filename);
            fp = fopen(FILE_NAME,"wb");
        }
        if(strcmp(filename,"FileUpload") == 0)
        {
            flag_upload = 1;
            filename = strtok(NULL," \n");
            strcpy(FILE_NAME,filename);
            FILE *f = fopen(FILE_NAME, "r");
            fseek(f, 0, SEEK_END);
            unsigned long len = (unsigned long)ftell(f);
            char size[1024];
            memset(size, 0, sizeof(size));
            sprintf(size,"%ld\n",len);
            strcat(write_buffer,size);
            fclose(f);
        }
        write(sockfd, write_buffer , strlen(write_buffer));

        data = read(sockfd, read_buffer, sizeof(read_buffer)-1);
        
        if(strcmp(read_buffer,"1") == 0)
        {
            size_t bytes_read; 
            int b,c;
            printf("Upload Accepted\n");
            handleVerify(FILE_NAME);
            sprintf(cresponse, "%s, ",hdata[0].filename);
            strcat(write_buffer,cresponse);
            for (c = 0 ; c < MD5_DIGEST_LENGTH ; c++)
            {
                sprintf(cresponse, "%02x",hdata[b].hash[c]);
                strcat(write_buffer,cresponse);
            }
            sprintf(cresponse, ", %s",ctime(&hdata[0].mtime));
            strcat(write_buffer,cresponse);
            write(sockfd , write_buffer , bytes_read);
            printf("%s\n",write_buffer);
            memset(write_buffer, 0, sizeof(write_buffer));
            fp = fopen(FILE_NAME,"rb");
            while(!feof(fp))
            {
                size_t bytes_read = fread(cresponse, 1, 1024, fp);
                cresponse[1024] = 0;
                memcpy(write_buffer,cresponse,bytes_read);
                write(sockfd , write_buffer , bytes_read);
                memset(write_buffer, 0, sizeof(write_buffer));
                memset(cresponse, 0, sizeof(cresponse));
            }
            memcpy(write_buffer,"~^~",3);
            write(sockfd , write_buffer , 3);
            memset(write_buffer, 0, sizeof(write_buffer));
            memset(read_buffer, 0, strlen(read_buffer));
            fclose(fp);
        }
        else if(strcmp(read_buffer,"0\n") == 0)
        {
            printf("Upload Denied\n");
            memset(read_buffer, 0,sizeof(read_buffer));
            continue;
        }
        else
        {
            while(1)
            {
                read_buffer[data] = 0;
                if(read_buffer[data-1] == '~' && read_buffer[data-3] == '~' && read_buffer[data-2] == '^')
                {
                    read_buffer[data-3] = 0;
                    if(flag_download == 1)
                    {
                        fwrite(read_buffer,1,data-3,fp);
                        fclose(fp);
                    }
                    else
                        strcat(cresponse,read_buffer);
                    memset(read_buffer, 0,strlen(read_buffer));
                    break;
                }
                else
                {
                    if(flag_download == 1)
                        fwrite(read_buffer,1,data,fp);
                    else
                        strcat(cresponse,read_buffer);
                    memset(read_buffer, 0,strlen(read_buffer));
                }
                data = read(sockfd, read_buffer, sizeof(read_buffer)-1);
                if(data < 0)
                    break;
            }
        }

        if(flag_download == 0)
            printf("%s\n",cresponse);
        else 
            printf("File Downloaded\n");

        if(data < 0)
            printf("\n Read error \n");
        memset(read_buffer, 0,sizeof(read_buffer));
        memset(write_buffer, 0,sizeof(write_buffer));
    }
    return 0;
}

int get_type(char *request)
{
    char copyrequest[100];
    strcpy(copyrequest,request);
    char *request_data = NULL;
    const char delim[] = " \n";
    request_data = strtok(copyrequest,delim);
    if(request_data != NULL)
    {   
        if(strcmp(request_data,"IndexGet") == 0)
            return 1;
        else if(strcmp(request_data,"FileHash") == 0)
            return 2;
        else if(strcmp(request_data,"FileDownload") == 0)
            return 3;
        else if(strcmp(request_data,"FileUpload") == 0)
            return 4;
        else if(strcmp(request_data, "Quit") == 0 || strcmp(request_data, "Q") == 0 || strcmp(request_data, "quit") == 0 || strcmp(request_data, "q") == 0 || strcmp(request_data, "exit") == 0 || strcmp(request_data, "Exit") == 0 ||  strcmp(request_data, "marjaa") == 0)
            return 5;
        else
            return -1;
    }
}

void IndexGet_handler(char *request)
{
    char *request_data = NULL;
    struct tm tm;
    time_t start_time , end_time;
    char *regexp;
    request_data = strtok(request,delim);
    request_data = strtok(NULL,delim);
    i = 0;
    if(request_data == NULL)
    {
        error = 1;
        sprintf(response,"Wrong Format\n");
        return;
    }
    else
    {
        if(strcmp(request_data,"--LongList") == 0)
        {
            request_data = strtok(NULL,delim);

            if(request_data != NULL)
            {
                error = 1;
                sprintf(response,"Wrong Format.\n");
                return;
            }
            else
            {
                DIR *dp;
                iter = 0; 
                struct dirent *ep;
                dp = opendir ("./");
                struct stat file_stat;
                iter = 0;
                if (dp != NULL)
                {
                    while ((ep = readdir (dp))!=NULL)
                    {
                        if(stat(ep->d_name,&file_stat) < 0)
                        return;
                        else
                        {
                            strcpy(pdata[iter].filename, ep->d_name);
                            pdata[iter].size = file_stat.st_size;
                            pdata[iter].mtime = file_stat.st_mtime;
                            if(S_ISDIR(file_stat.st_mode))
                                strcpy(pdata[iter].type,"dir");
                            else if(S_ISLNK(file_stat.st_mode))
                                strcpy(pdata[iter].type,"lnk");
                            else
                                strcpy(pdata[iter].type,"-");
                            iter++;
                        }
                    }
                    closedir (dp);
                }
                else
                {
                    printf("Couldn't open the directory");
                }
            }
        }
        else if(strcmp(request_data,"--ShortList") == 0)
        {
            int no_of_arg = 1;
            request_data = strtok(NULL,delim);
            if(request_data == NULL)
            {
                error = 1;
                sprintf(response,"Wrong Format.\n");
                return;
            }
            while(request_data != NULL)
            {
                if(no_of_arg >= 3)
                {
                    error = 1;
                    sprintf(response,"Wrong Format.\n");
                    return;
                }
                if (strptime(request_data, "%d-%b-%Y-%H:%M:%S", &tm) == NULL)
                {
                    error = 1;
                    sprintf(response,"The correct format is:\nDate-Month-Year-hrs:min:sec\n");
                    return;
                }
                if(no_of_arg == 1)
                    start_time = mktime(&tm);
                else
                    end_time = mktime(&tm);
                no_of_arg++;
                request_data = strtok(NULL,delim);
            }
                DIR *dp;
                i = 0; 
                struct dirent *ep;
                dp = opendir (".");
                struct stat file_stat;
                if (dp != NULL)
                {
                    while ((ep = readdir (dp))!=NULL)
                    {
                        if(stat(ep->d_name,&file_stat) < 0)
                        return;
                        else if(difftime(file_stat.st_mtime,start_time) > 0 && difftime(end_time,file_stat.st_mtime) > 0)
                        {
                            strcpy(pdata[i].filename, ep->d_name);
                            pdata[i].size = file_stat.st_size;
                            pdata[i].mtime = file_stat.st_mtime;
                            if(S_ISDIR(file_stat.st_mode))
                                strcpy(pdata[i].type,"dir");
                            else if(S_ISLNK(file_stat.st_mode))
                                strcpy(pdata[i].type,"lnk");
                            else
                                strcpy(pdata[i].type,"-");
                            i++;
                        }
                    }
                    closedir (dp);
                }
                else
                {
                    printf("Couldn't open the directory");
                }
        }
        else if(strcmp(request_data,"--RegEx") == 0)
        {
            regex = 1;
            i = 0;
            request_data = strtok(NULL,delim);
            if(request_data == NULL)
            {
                printf("Wrong Format.\n");
                _exit(1);
            }
            regexp = request_data;
            request_data = strtok(NULL,delim);
            if(request_data != NULL)
            {
                printf("Wrong Format.\n");
                _exit(1);
            }
            DIR *dp;
            struct dirent *ep;
            dp = opendir (".");
                   
            if (dp != NULL)
            {
                while ((ep = readdir (dp))!=NULL)
                {
                    regex_t reg;
                    int rv = regcomp(&reg,regexp,0);
                    if(rv!=0)
                    {
                        printf("regex failed");
                        return;
                    }    
                    struct stat file_stat;
                    if(stat(ep->d_name,&file_stat) < 0)
                        return;
                    else 
                    {
                        if(regexec(&reg, ep->d_name, 0, NULL, 0)==0)
                        {
                            strcpy(pdata[i].filename, ep->d_name);
                            pdata[i].size = file_stat.st_size;
                            pdata[i].mtime = file_stat.st_mtime;
                            if(S_ISDIR(file_stat.st_mode))
                                strcpy(pdata[i].type,"dir");
                            else if(S_ISLNK(file_stat.st_mode))
                                strcpy(pdata[i].type,"lnk");
                            else
                                strcpy(pdata[i].type,"-");
                            i++;
                        }
                    }
                }
                closedir(dp);
            }
            else
            {
                    printf("Couldn't open the directory");
            }
        }
    }    
   
}


void FileDownload_handler(char *request)
{
    char *request_data = NULL;
    request_data = strtok(request,delim);
    request_data = strtok(NULL,delim);
    if(request_data == NULL)
    {
        error = 1;
        sprintf(response,"ERROR: Wrong Format. The correct format is:\nFileDownload <file_name>\n");
        return;
    }
    strcpy(fileDownloadName,request_data);
    request_data = strtok(NULL,delim);
    if(request_data != NULL)
    {
        error = 1;
        sprintf(response,"ERROR: Wrong Format. The correct format is:\nFileDownload <file_name>\n");
        return;
    }
}

void FileHash_handler(char *request)
{
    char *request_data = NULL;
    request_data = strtok(request,delim);
    request_data = strtok(NULL,delim);
    if(request_data == NULL)
    {
        error = 1;
        sprintf(response,"ERROR: Wrong Format\n");
    }
    while(request_data != NULL)
    {
        if(strcmp(request_data,"--checkall") == 0)
        {
            request_data = strtok(NULL,delim);

            if(request_data != NULL)
            {
                error = 1;
                sprintf(response,"ERROR: Wrong Format. The correct format is:\nFileHash --checkall\n");
                return;
            }
            else
            {
                handleCheckAll();
            }

        }
        else if(strcmp(request_data,"--verify") == 0)
        {
            request_data = strtok(NULL,delim);
            if(request_data == NULL)
            {
                error = 1;
                sprintf(response,"ERROR: Wrong Format. The correct format is:\nFileHash --verify <filename>\n");
                return;
            }
            char *filename = request_data;
            request_data = strtok(NULL,delim);
            if(request_data != NULL)
            {
                error = 1;
                sprintf(response,"ERROR: Wrong Format. The correct format is:\nFileHash --verify <filename>\n");
                return;
            }
            else
                handleVerify(filename);
        }
    }
}

int handleCheckAll()
{
    unsigned char c[MD5_DIGEST_LENGTH];
    DIR *dp;
    i = 0;
    int a;
    struct dirent *ep;
    dp = opendir ("./");
    struct stat fileStat;

    if (dp != NULL)
    {
        while (ep = readdir (dp))
        {
            if(stat(ep->d_name,&fileStat) < 0)
                return 1;
            else
            {
                char *filename=ep->d_name;
                strcpy(hdata[i].filename,ep->d_name);
                hdata[i].mtime = fileStat.st_mtime;
                FILE *inFile = fopen (filename, "r");
                MD5_CTX mdContext;
                int bytes;
                unsigned char data[1024];

                if (inFile == NULL) {
                    error = 1;
                    sprintf (response,"%s can't be opened.\n", filename);
                    return 0;
                }

                MD5_Init (&mdContext);
                while ((bytes = fread (data, 1, 1024, inFile)) != 0)
                    MD5_Update (&mdContext, data, bytes);
                MD5_Final (c,&mdContext);
                for(a = 0; a < MD5_DIGEST_LENGTH; a++)
                    hdata[i].hash[a] = c[a];
                fclose (inFile);
                i++;
            }
        }
    }
    else
    {
        error = 1;
        sprintf(response,"Couldn't open the directory");
    }
}

int handleVerify(char *file)
{
    unsigned char c[MD5_DIGEST_LENGTH];
    DIR *dp;
    i = 0;
    int a;
    struct dirent *ep;
    dp = opendir ("./");
    struct stat fileStat;

    if (dp != NULL)
    {
        while (ep = readdir (dp))
        {
            if(stat(ep->d_name,&fileStat) < 0)
                return 1;
            else if(strcmp(file,ep->d_name) == 0)
            {
                char *filename=ep->d_name;
                strcpy(hdata[i].filename,ep->d_name);
                hdata[i].mtime = fileStat.st_mtime;
                FILE *inFile = fopen (filename, "r");
                MD5_CTX mdContext;
                int bytes;
                unsigned char data[1024];

                if (inFile == NULL) {
                    error = 1;
                    sprintf(response,"%s can't be opened.\n", filename);
                    return 0;
                }

                MD5_Init (&mdContext);
                while ((bytes = fread (data, 1, 1024, inFile)) != 0)
                    MD5_Update (&mdContext, data, bytes);
                MD5_Final (c,&mdContext);
                for(a = 0; a < MD5_DIGEST_LENGTH; a++)
                    hdata[i].hash[a] = c[a];
                fclose (inFile);
                i++;
            }
            else
                continue;
        }
    }
    else
    {
        error = 1;
        sprintf(response,"Couldn't open the directory");
    }
}
