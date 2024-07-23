
#define TLS_AMALGAMATION
#include "tlse.c"

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("no socket");
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(443);

    int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind");
        return 1;
    }

    listen(sock, 3);

    SSL *ctx = SSL_CTX_new(SSLv3_server_method());
    SSL_CTX_use_PrivateKey_file(ctx, "cert/private.key", SSL_SERVER_RSA_KEY);
    SSL_CTX_use_certificate_file(ctx, "cert/cert.crt", SSL_SERVER_RSA_CERT);

    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key not loaded\n");
        return -2;
    }

    while (1)
    {
        struct sockaddr_in client;
        socklen_t c;

        int cl = accept(sock, (struct sockaddr *)&client, &c);

        SSL *clientctx = SSL_new(ctx);
        SSL_set_fd(clientctx, cl);
        SSL_accept(clientctx);

        char client_message[0xFFFF];
        int read_size;
        const char msg[] = "HTTP/1.1 200 OK\r\nContent-length: 31\r\nContent-type: text/plain\r\n\r\nHello world from TLSe (TLS 1.2)";


        while ((read_size = SSL_read(clientctx, client_message, sizeof(client_message))) >= 0) {
                fwrite(client_message, read_size, 1, stdout);
                
                if (SSL_write(clientctx, msg, strlen(msg)) < 0)
                    fprintf(stderr, "Error in SSL write\n");
                break;
            }

        SSL_shutdown(clientctx);

        shutdown(cl, SHUT_RDWR);
        close(cl);
        SSL_free(clientctx);
    }

    SSL_CTX_free(ctx);

    return 0;
}
