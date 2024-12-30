#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // Ağ adresi dönüşümleri ve sockaddr_in için.
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <time.h>

#define PACKET_SIZE 64

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Kullanım: %s <server-ip> <destination-ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    const char *dest_ip = argv[2];
    int sockfd;
    struct sockaddr_in server_addr;

    // TCP soketi oluştur,IPv4 adres ailesi
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Soket oluşturma başarısız oldu");
        exit(EXIT_FAILURE);
    }
    
    // Sunucu adres yapısını doldur
    server_addr.sin_family = AF_INET; // Adres ailesi 
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bağlantı başarısız oldu");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Hedef IP sunucuya gönderiliyor: %s\n", dest_ip);
    send(sockfd, dest_ip, strlen(dest_ip) + 1, 0);
                        //Hedef IP'nin uzunluğu  

    close(sockfd);
    return 0;
}
