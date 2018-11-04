#include "crack.h"
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <thread>
#include <cmath>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <crypt.h>

std::vector<std::thread> listThreads;
std::vector<char> listPasswds;

unsigned int convToNetwork(unsigned int currNum)
{
    currNum = htonl(currNum);
    return currNum;

}

unsigned int convToHost(unsigned int currNum)
{
    currNum = ntohl(currNum);
    return currNum;

}

void crackHelper(unsigned int index, char msgPasswd[], Message &tempMessage)
{
	char * output = new char[64];
	crack(msgPasswd, output);
	//std::cout << "[DEBUG] output is: " << output <<std::endl;
	std::string plainPass(output);
    //std::cout << "[DEBUG] plainPass is: " << plainPass << std::endl;
	strcpy(tempMessage.passwds[index], plainPass.c_str());
	//delete[] output;

}


int main(int argc, char *argv[])
{
    int status;
    listThreads.reserve(64);
    listPasswds.reserve(64);

    while(true)
    {
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0){
            std::cout << "Error on sockfd\n";
            exit(-1);
        }

        struct sockaddr_in remote_addr;
        // Zero out array to check for any wrong things
        bzero ((char*) &remote_addr, sizeof(remote_addr));

        // Set family of our object
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(get_multicast_port());

         if (bind(sockfd, (struct sockaddr *) &remote_addr, sizeof(remote_addr)) < 0) exit(-1);
         struct ip_mreq multicastRequest;
         multicastRequest.imr_multiaddr.s_addr = get_multicast_address();
         multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
         if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
         (void *) &multicastRequest, sizeof(multicastRequest)) < 0)
         exit(-1);

        //remote_addr.sin_addr.s_addr = get_multicast_address();
        // Ensure we are transmitting correct order over network
        socklen_t len = sizeof(remote_addr);
		Message outboundMsg;
        Message msg;
        unsigned int listSize;
        //char * output;

        status = recvfrom(sockfd, &msg, sizeof(Message), 0, (struct sockaddr *) &remote_addr, &len);
        if (status < 0)
        {
            std::cout << "Receive error received" << std::endl;
            exit(-1);
        }
        //std::cout << "[DEBUG] Message received \n";


        
        close(sockfd);
        //std::cout << "[DEBUG] Closed socket" << std::endl;
        std::string myCruzID = msg.cruzid;

        // Num passwords is where we will split up lists to multiple servers

        if(myCruzID == "klhoang")
        {   

	
        	//unsigned int threadCounter = 0;
            listSize = convToHost(msg.num_passwds);
            strcpy(outboundMsg.cruzid, msg.cruzid);
            outboundMsg.num_passwds = msg.num_passwds;
            strcpy(outboundMsg.hostname, msg.hostname);
            outboundMsg.port = msg.port;

            //std::cout << "Size is " << listSize << std::endl;
            for(unsigned int i=0; i<listSize; ++i)
            {
        		listThreads.push_back(std::thread(crackHelper, i, std::ref(msg.passwds[i]), std::ref(outboundMsg)));

            }

	        //Begin cracking!
	        for(auto &myThread : listThreads){
	            if(myThread.joinable())
	                myThread.join();
	        }

            int sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd_tcp < 0)
            {
                std::cout << "Socket_tcp error" << std::endl;
                exit(-1);
            }



            struct hostent *server = gethostbyname(msg.hostname);
            if (server == NULL)
            {
                std::cout << "Server is null" << std::endl;
                exit(-1);
            }
            
            struct sockaddr_in server_addr;
            bzero((char *) &server_addr, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);

            server_addr.sin_port = msg.port;
            if (connect(sockfd_tcp,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0)
            {
                std::cout << "Connecting error\n";
                exit(-1);

            }

            status = write(sockfd, &outboundMsg, sizeof(Message));
            //status = send(sockfd, (void*) &outboundMsg, sizeof(Message), 0);
            if (status < 0)
            {
                std::cout << "Send error received" << std::endl;
                exit(-1);
            }
            listThreads.clear();
            //std::cout <<"[DEBUG] Message sent! \n";

            close(sockfd_tcp);
            //std::cout <<"[DEBUG] TCP socket closed \n";

            

        }


    }
    return 0;

}