/* This program calculates the Key for two persons
using the Diffie-Hellman Key exchange algorithm */
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

//openssl dgst -sha256 -out signature.txt dh.c
//scp dh.c 172.26.37.44:dh.c
//  ./dh 172.26.37.44 7800
int modulo(int a, int b, int n);
long long int power(long long int a, long long int b, long long int p);
int hexadecimalToDecimal(char hexVal[]);

int main(int argc, char ** argv)
{
    int sockfd, portno, n, g, p;
    long long int a, b, x, y, ka, kb;
    struct sockaddr_in serv_addr;
    struct hostent * server;
    FILE * fp;

    fp = fopen("signature.txt","r");

    char buffer[512];
    char fread[256];
    char bmod[256];


    //get the b here
    fgets(fread,3,fp);
    printf("%s\n",fread);
    //b = (long long int)strtol(fread[0], NULL, 16) + (long long int)strtol(fread[1], NULL, 16);
    //b = (int)strtol(fread, NULL, 16);
    b = (long long int)hexadecimalToDecimal(fread);
    printf("b = %lli\n", b);
    fclose(fp);

    g = 15;
    p = 97;

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    portno = atoi(argv[2]);

    /* Translate host name into peer's IP address ;
     * This is name translation service by the operating system */
    server = gethostbyname(argv[1]);

    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    /* Building data structures for socket */

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy(server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);

    /* Create TCP socket -- active open
     * Preliminary steps: Setup: creation of active open socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        exit(EXIT_FAILURE);
    }

    n = write(sockfd, "zpullman\n", strlen("zpullman\n"));
    if (n < 0)
    {
        perror("ERROR writing to socket");
        exit(EXIT_FAILURE);
    }


    //send bmod
    x = modulo(g,b,p);
    sprintf(bmod, "%lld", x);
    printf("bmod %s\n",bmod);
    n = write(sockfd, bmod, strlen(bmod));
    if (n < 0)
    {
        perror("ERROR writing to socket");
        exit(EXIT_FAILURE);
    }

    /* Do processing */
    while (1)
    {
      /*
      ** dh need to strstr buffer to get hashed dh, also use strtol to get
      ** str to long.
      */
        fgets(buffer, 511, stdin);

        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
        {
            perror("ERROR writing to socket");
            exit(EXIT_FAILURE);
        }

        if (!strncmp(buffer, "GOODBYE-CLOSE-TCP", 17))
            break;

        n = read(sockfd, buffer, 511);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        buffer[n] = 0;
        
        printf("%s\n", buffer);
    }

    close(sockfd);

    return 0;
}

//https://stackoverflow.com/questions/8496182/calculating-powa-b-mod-n?fbclid=IwAR36P5QO3kYivyR1OlaiW8NoLTw9OkdgSKcjEt0aqaQp8KkiyDAmUt7_MkM
int modulo(int a, int b, int n){
   long long x=1, y=a;
   while (b > 0) {
       if (b%2 == 1) {
           x = (x*y) % n; // multiplying with base
       }
       y = (y*y) % n; // squaring the base
       b /= 2;
   }
   return x % n;
}

long long int power(long long int a, long long int b,
                                     long long int p)
{
    if (b == 1)
        return a;

    else
        //return (((long long int)pow(a, b)) % p);
        return 1;
}

// https://www.geeksforgeeks.org/program-hexadecimal-decimal/
// Function to convert hexadecimal to decimal
int hexadecimalToDecimal(char hexVal[])
{
    int len = strlen(hexVal);

    // Initializing base value to 1, i.e 16^0
    int base = 1;

    int dec_val = 0;

    // Extracting characters as digits from last character
    for (int i=len-1; i>=0; i--)
    {
        // if character lies in '0'-'9', converting
        // it to integral 0-9 by subtracting 48 from
        // ASCII value.
        if (hexVal[i]>='0' && hexVal[i]<='9')
        {
            dec_val += (hexVal[i] - 48)*base;

            // incrementing base by power
            base = base * 16;
        }

        // if character lies in 'A'-'F' , converting
        // it to integral 10 - 15 by subtracting 55
        // from ASCII value
        else if (hexVal[i]>='A' && hexVal[i]<='F')
        {
            dec_val += (hexVal[i] - 55)*base;

            // incrementing base by power
            base = base*16;
        }
    }

    return dec_val;
}
