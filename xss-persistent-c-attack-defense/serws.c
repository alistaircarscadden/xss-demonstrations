#define _GNU_SOURCE

#include "al.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CONNECTIONS (1000)
#define MAX_PATH_L (256)
#define MAX_RECV_L (128 * 256)
#define MAX_THREADS (8)
#define MAX_GET_TOKENS (8)
#define MAX_POST_FORM_DATA_ELEMENTS (16)

const char *pwd = ".";

array_list user_posts;

void *handle_request(void *arg);

int main(int argc, char *argv[])
{
    int listening_socket;
    char *port = "8080";

    struct addrinfo hints;
    struct addrinfo *res;

    // getaddrinfo for host
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, port, &hints, &res) != 0)
    {
        printf("getaddrinfo() error");
        exit(1);
    }
    // socket and bind
    struct addrinfo *p;

    for (p = res; p != NULL; p = p->ai_next)
    {
        listening_socket = socket(p->ai_family, p->ai_socktype, 0);
        if (listening_socket == -1)
            continue;
        if (bind(listening_socket, p->ai_addr, p->ai_addrlen) == 0)
            break;
    }
    if (p == NULL)
    {
        printf("socket() or bind()");
        exit(1);
    }

    freeaddrinfo(res);

    if (listen(listening_socket, MAX_CONNECTIONS) != 0)
    {
        printf("listen() error");
        exit(1);
    }

    printf("Listening on port %s\nWorking from %s\n", port, ".");

    pthread_t threads[MAX_THREADS];
    int accept_fds[MAX_THREADS];
    int free_thread = 0;

    for (int i = 0; i < MAX_THREADS; i++)
    {
        threads[i] = 0;
        accept_fds[i] = -1;
    }

    alist_create(&user_posts, 10);

    while (1)
    {
        free_thread = -1;
        for (int i = 0; i < MAX_THREADS; i++)
        {
            if (accept_fds[i] != -1 && pthread_tryjoin_np(threads[i], NULL) == 0)
            {
                // thread has finished
                accept_fds[i] = -1;
            }

            if (free_thread == -1 && accept_fds[i] == -1)
            {
                free_thread = i;
            }
        }

        if (free_thread != -1)
        {
            struct sockaddr_in clientaddr;
            socklen_t addrlen = sizeof(clientaddr);
            accept_fds[free_thread] = accept(listening_socket, (struct sockaddr *)&clientaddr, &addrlen);

            if (accept_fds[free_thread] < 0)
            {
                printf("accept() error");
                continue;
            }

            // only can start new thread if there is one available
            if (pthread_create(&threads[free_thread], NULL, &handle_request, &accept_fds[free_thread]) != 0)
            {
                printf("pthread_create() error");
                exit(-1);
            }
            printf("Thread # %d handling request\n", free_thread);
        }
        else
        {
            printf("No threads available for incomming connection\n");
        }
    }

    return 0;
}

#ifdef DEFENSE
char *html_encode(char *str)
{
    
    int capacity = strlen(str);
    int increment = capacity / 2;
    capacity = capacity + increment;
    char *encoded = malloc(capacity);
    char *r = str;
    char *w = encoded;

    while (*r != 0)
    {
        int length = w-encoded;
        // ensure there is always at least 6 bytes free
        if (length > capacity-6)
        {
            encoded = realloc(encoded, capacity + increment);
            capacity += increment;
        }

        switch (*r)
        {
            case '<':
                strcpy(w, "&lt;");
                w += 4;
                break;
            case '>':
                strcpy(w, "&gt;");
                w += 4;
                break;
            case '&':
                strcpy(w, "&amp;");
                w += 5;
                break;
            default:
                *w = *r;
                w += 1;
                break;
        }
        r++;
    }

    *r = 0;
    return encoded;
}
#endif

char hex_decode(char a, char b)
{
    char out = 0;

    if ('0' <= a && a <= '9')
    {
        out += (a - '0') * 16;
    }
    else if ('A' <= a && a <= 'F')
    {
        out += (a - 'A' + 10) * 16;
    }

    if ('0' <= b && b <= '9')
    {
        out += (b - '0');
    }
    else if ('A' <= b && b <= 'F')
    {
        out += (b - 'A' + 10);
    }

    return out;
}

void http_decode_i(char *str)
{
    char *r = str;
    char *w = str;

    while (*r != 0)
    {
        if (*r == '+')
        {
            *w = ' ';
        }
        else if (*r == '%')
        {
            if (*(r + 1) != 0 && *(r + 2) != 0)
            {
                *w = hex_decode(*(r + 1), *(r + 2));
                r += 2;
            }
        }
        else
        {
            *w = *r;
        }
        r++;
        w++;
    }

    *w = 0;
}

void send_index(int socket_fd)
{
    send(socket_fd, "HTTP/1.0 200 OK\n\n", 17, 0);
    send(socket_fd, "<html><body><h1>User Posts</h1><a href=\"post.html\">Make a post</a>", 66, 0);

    for (int i = 0; i < user_posts.length; i++)
    {
        send(socket_fd, "<p>", 3, 0);
        char *msg = alist_get(&user_posts, i);
        send(socket_fd, msg, strlen(msg), 0);
        send(socket_fd, "</p>", 4, 0);
    }

    send(socket_fd, "</body></html>", 14, 0);
}

void handle_get(int socket_fd, char *resource)
{
    char send_buf[MAX_RECV_L];
    char path[MAX_PATH_L];

    strncpy(path, pwd, MAX_PATH_L);
    strncat(path, resource, MAX_PATH_L);

    // override normal behaviour to send special index page
    if (strcmp(path, "./index.html") == 0)
    {
        send_index(socket_fd);
        return;
    }

    int fd;
    if ((fd = open(path, O_RDONLY)) != -1)
    {
        int read_count;
        send(socket_fd, "HTTP/1.0 200 OK\n\n", 17, 0);
        while ((read_count = read(fd, send_buf, MAX_RECV_L)) > 0)
        {
            send(socket_fd, send_buf, read_count, 0);
        }
    }
    else
    {
        send(socket_fd, "HTTP/1.0 404 Not Found\n", 23, 0);
    }
}

void handle_post(int socket_fd, char *resource, char *request)
{
    if (strcmp(resource, "/post.html") != 0)
    {
        printf("Can't handle a POST to %s\n", resource);
    }

    char *start = strstr(request, "\r\n\r\n");
    start = strchr(start, '=');
    start++;

    printf("User Posted: %s\n", start);
    http_decode_i(start);
    printf("User Posted D: %s\n", start);
    char *copy = malloc(strlen(start) + 1);
    strcpy(copy, start);

#ifdef DEFENSE
    char *escaped = html_encode(copy);
    free(copy);
    copy = escaped;
#endif

    alist_push(&user_posts, copy);

    send(socket_fd, "HTTP/1.1 303 See Other\r\nLocation: localhost:8080/index.html", 35, 0);
}

void *handle_request(void *arg)
{
    int socket_fd = *(int *)arg;

    char recv_buf[MAX_RECV_L];
    char recv_buf_orig[MAX_RECV_L];

    char *request_tokens[MAX_GET_TOKENS];

    int recv_count = recv(socket_fd, recv_buf, MAX_RECV_L - 1, 0);
    recv_buf[recv_count] = 0;
    memcpy(recv_buf_orig, recv_buf, recv_count);

    if (recv_count <= 0)
    {
        printf("error: recv error or socket closed.\n");
        goto ret;
    }

    printf("%s\n", recv_buf);

    char *tok = strtok(&recv_buf[0], " \t\n\r");
    request_tokens[0] = tok;
    for (int i = 1; i < MAX_GET_TOKENS && (tok = strtok(NULL, " \t\n\r")) != NULL; i++)
    {
        request_tokens[i] = tok;
    }

    if (strcmp(request_tokens[2], "HTTP/1.0") != 0 && strcmp(request_tokens[2], "HTTP/1.1") != 0)
    {
        printf("Can't deal with \"%s\", expected HTTP/1.0 or HTTP/1.1\n", request_tokens[2]);
        send(socket_fd, "HTTP/1.0 400 Bad Request\n", 25, 0);
        goto ret;
    }

    char *resource = request_tokens[1];
    if (strcmp(resource, "/") == 0)
    {
        resource = "/index.html";
    }

    if (strcmp(request_tokens[0], "GET") == 0)
    {
        handle_get(socket_fd, resource);
    }
    else if (strcmp(request_tokens[0], "POST") == 0)
    {
        handle_post(socket_fd, resource, recv_buf_orig);
    }
    else
    {
        printf("Can't deal with \"%s\", expected GET etc.\n", request_tokens[0]);
    }

ret:
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
    return NULL;
}