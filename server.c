#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;


double time_diff_ms(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_usec - start.tv_usec) / 1000.0;
}


void write_log(const char* client_ip, const char* file, int status, double duration_ms) {
    FILE* logf;

    pthread_mutex_lock(&log_lock);
    logf = fopen("logs/server.log", "a");
    if (!logf) {
        perror("logfile");
        pthread_mutex_unlock(&log_lock);
        return;
    }

    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    fprintf(logf,
        "[%04d-%02d-%02d %02d:%02d:%02d] %s \"%s\" %d (%.2f ms)\n",
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec,
        client_ip, file, status, duration_ms);

    fclose(logf);
    pthread_mutex_unlock(&log_lock);
}


void* handle_client(void* arg) {
    int client_fd = ((int*)arg)[0];
    struct sockaddr_in client_addr = *((struct sockaddr_in*)((int*)arg + 1));
    free(arg);

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

    char buffer[1024] = {0};
    recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    int status = 0;
    char requested[256] = {0};

    if (strncmp(buffer, "GET ", 4) == 0) {
        char* f = buffer + 4;
        char* sp = strchr(f, ' ');
        if (sp) *sp = 0;
        if (strcmp(f, "/") == 0) f = "/index.html";
        snprintf(requested, sizeof(requested), "%s", f);

        char path[256];
        snprintf(path, sizeof(path), "www%s", f);

        int opened_fd = open(path, O_RDONLY);
        if (opened_fd < 0) {
            char not_found[] = "HTTP/1.1 404 Not Found\r\n"
                               "Content-Type: text/plain\r\n\r\n"
                               "404 Not Found";
            send(client_fd, not_found, strlen(not_found), 0);
            status = 404;
        } else {
            struct stat st;
            fstat(opened_fd, &st);
            char header[128];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n\r\n",
                     st.st_size);
            send(client_fd, header, strlen(header), 0);
            sendfile(client_fd, opened_fd, NULL, st.st_size);
            close(opened_fd);
            status = 200;
        }
    }

    gettimeofday(&end_time, NULL);
    double elapsed_ms = time_diff_ms(start_time, end_time);

    
    write_log(client_ip, requested, status, elapsed_ms);

    close(client_fd);
    pthread_exit(NULL);
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(s, 10) < 0) {
        perror("listen");
        return 1;
    }

    printf(" Server listening on http://localhost:8080\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        int client_fd = accept(s, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd < 0) continue;

        int* arg = malloc(sizeof(int) + sizeof(struct sockaddr_in));
        ((int*)arg)[0] = client_fd;
        memcpy((void*)((int*)arg + 1), &client_addr, sizeof(client_addr));

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, arg);
        pthread_detach(thread);
    }

    close(s);
    return 0;
}

