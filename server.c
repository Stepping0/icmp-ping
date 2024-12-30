#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define PACKET_SIZE 64

uint16_t checksum(void *b, int len) {
    uint16_t *buf = b; //Veriyi 16 bitlik parçalara ayırır
    unsigned sum = 0;
    uint16_t result;

    //paket içeriğini 16 bitlik parçalar halinde toplar
    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;     //Bellekteki sıradaki 16 bitlik (uint16_t) veriyi alır ve toplar. Ardından buf işaretçisini bir sonraki 16 bitlik bloğa kaydırır.
    if (len == 1)
        sum += *(uint8_t *)buf;  //tek bayt kaldıysa eklenir
    
    //taşmalar dahil edilir
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;   //bitlerin tümleci alınır
    return result;
}

                                //paket uzunluğu, zaman damgası
void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
                                            // Ethernet başlığı (14 bayt) atlanır ve IP başlığına ulaşılır.
    struct iphdr *ip_hdr = (struct iphdr *)(packet + 14);   // Ethernet başlığından sonra IP başlığını oku
    struct icmphdr *icmp_hdr = (struct icmphdr *)(packet + 14 + ip_hdr->ihl * 4);    // IP başlığından sonra ICMP başlığını oku

    // Eğer ICMP cevap paketi (Echo Reply) geldiyse
    if (icmp_hdr->type == ICMP_ECHOREPLY) {
        struct timeval *start = (struct timeval *)args;
        struct timeval end;
        gettimeofday(&end, NULL);

        long elapsed_time = (end.tv_sec - start->tv_sec) * 1000 + (end.tv_usec - start->tv_usec) / 1000;

        printf("64 bytes from %s: icmp_seq=%d ttl=%d time=%ld ms\n",
               inet_ntoa(*(struct in_addr *)&ip_hdr->saddr),
               icmp_hdr->un.echo.sequence,
               ip_hdr->ttl,
               elapsed_time);

        exit(0);
    }
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char dest_ip[INET_ADDRSTRLEN];   // Hedef IP adresi için bir tampon

    // Sunucu soketini oluştur
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket oluşturulamadı");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    // Sunucu adres yapısını doldur
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind başarısısz ldu");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("Dİnleme başarısız");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server 12345. portta dinleniyor...\n");

    socklen_t addr_len = sizeof(client_addr);
    if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
        perror("Kabul edilemedi");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    recv(client_sock, dest_ip, sizeof(dest_ip), 0);
    printf("İstemciden hedef IP alındı: %s\n", dest_ip);

    close(client_sock);
    close(server_sock);

    // Ağ arayüzünü pcap ile aç
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live("eth0", BUFSIZ, 1, 1000, errbuf);
    if (!handle) {
        fprintf(stderr, "Cihaz açılırken hata oluştu: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in dest_addr;

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        perror("Soket oluşturma başarısız oldu");
        exit(EXIT_FAILURE);
    }

    // Hedef adres yapısını doldur
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(dest_ip);

    char sendbuf[PACKET_SIZE];
    struct icmphdr *icmp_hdr = (struct icmphdr *)sendbuf;

    // ICMP paketi için tamponu sıfırla
    memset(sendbuf, 0, PACKET_SIZE);

    // ICMP başlığını doldur
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0;
    icmp_hdr->checksum = 0;
    icmp_hdr->un.echo.id = getpid();
    icmp_hdr->un.echo.sequence = 1;
    icmp_hdr->checksum = checksum(icmp_hdr, sizeof(struct icmphdr));

    struct timeval start;
    gettimeofday(&start, NULL);

    if (sendto(sockfd, sendbuf, sizeof(struct icmphdr), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) <= 0) {
        perror("Sendto başarısız oldu");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("PING %s (%s): 64 bytes of data\n", dest_ip, dest_ip);

    // Gelen paketleri yakala ve işleyiciye gönder
    pcap_loop(handle, -1, packet_handler, (u_char *)&start);

    close(sockfd);
    pcap_close(handle);

    return 0;
}
