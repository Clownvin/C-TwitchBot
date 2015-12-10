#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "string_utils.h"


struct connection {
  int status;
  int socketfd;
  bool connected;
  char* address;
  char* port;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
};

int connect_to_irc(struct connection* to_connect); // Returns status

int close_connection(struct connection* to_close);

int send_line(struct connection send_connection, char * buffer);

int send_command(struct connection send_connection, char * command, char * message);

int send_message(struct connection send_connection, char * channel, char * message);

int send_whisper(struct connection send_connection, char * channel, char * user, char * message);

int read_line(struct connection read_connection, char * buffer);

int connect_to_irc(struct connection* to_connect) {
  if (to_connect->connected) {
    printf("Socket already connected.\n");
    return -1;
  }
  memset(&to_connect->host_info, 0, sizeof(to_connect->host_info));

  to_connect->host_info.ai_family = AF_UNSPEC; // IP Version not specified.
  to_connect->host_info.ai_socktype = SOCK_STREAM; // SOCK_STREAM for TCP, SOCK_DGRAM for UDP

  to_connect->status = getaddrinfo(to_connect->address, to_connect->port, &to_connect->host_info, &to_connect->host_info_list);
  if (to_connect->status != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(to_connect->status));
    return to_connect->status;
  }

  printf("Creating a socket...\n");
  to_connect->socketfd = socket(to_connect->host_info_list->ai_family, to_connect->host_info_list->ai_socktype, to_connect->host_info_list->ai_protocol);
  if (to_connect->socketfd == -1) {
    printf("socket error\n");
    return -1;
  }
  printf("Connecting...\n");
  to_connect->status = connect(to_connect->socketfd, to_connect->host_info_list->ai_addr, to_connect->host_info_list->ai_addrlen);
  if (to_connect->status == -1) {
    printf("connect error.\n");
    return to_connect->status;
  }
  to_connect->connected = true;
  printf("Connected. %d\n", to_connect->connected ? 0 : 1);
  return 0;
}

int close_connection(struct connection* to_close) {
  to_close->connected = false;
  return close(to_close->socketfd);
}

int send_line(struct connection send_connection, char * buffer) {
  if (!send_connection.connected) {
    printf("Socket not connected.\n");
    return -1;
  }
  int len;
  len = strlen(buffer);
  if (buffer[len -1] != '\n') { // Doesn't already end in newline.
    char* p = realloc(buffer, len + 4);
    if (!p) {
      printf("Realloc error.");
    } else {
      free(buffer);
      buffer = p;
      strcat(buffer, "\r\n"); // Attach newline sequence to end.
      len += 4;
    }
  }
  ssize_t bytes_sent;
  printf("Sending buffer: %s", buffer); // Don't need newline because buffer should have it on the end by this point.
  bytes_sent = send(send_connection.socketfd, buffer, len, 0);
  return bytes_sent == len ? 0 : -1;
}

int send_command(struct connection send_connection, char * command, char * message) {
  if (!send_connection.connected) {
    printf("Socket not connected.\n");
    return -1;
  }
  char combined[500];
  memset(combined, '\0', 500);
  strcpy(combined, command);
  strcat(combined, " ");
  strcat(combined, message);
  strcat(combined, "\r\n");
  return send_line(send_connection, combined);
}

int send_message(struct connection send_connection, char * channel, char * message) {
  if (!send_connection.connected) {
    printf("Socket not connected.\n");
    return -1;
  }
  char combined[500];
  memset(combined, '\0', 500);
  strcpy(combined, "PRIVMSG ");
  strcat(combined, channel);
  strcat(combined, " :");
  strcat(combined, message);
  strcat(combined, "\r\n");
  return send_line(send_connection, combined);
}

int send_whisper(struct connection send_connection, char * channel, char * user, char * message) {
  if (!send_connection.connected) {
    printf("Socket not connected.\n");
    return -1;
  }
  char combined[500];
  memset(combined, '\0', 500);
  strcpy(combined, "/w ");
  strcat(combined, user);
  strcat(combined, " ");
  strcat(combined, message);
  return send_message(send_connection, channel, combined);
}

int read_line(struct connection read_connection, char * buffer) {
  if (!read_connection.connected) {
    printf("Socket not connected.\n");
    return -1;
  }
  char readbuff[1]; // 1 byte of read.
  ssize_t bytes_recieved;
  do {
    bytes_recieved = recv(read_connection.socketfd, readbuff, 1, 0);
    if (bytes_recieved == 0) {
      printf("host shut down.\n");
      return -1;
    }
    if (bytes_recieved == -1) {
      printf("read error\n");
      return -1;
    }
    if (readbuff[0] == '\r' || readbuff[0] == '\n') {
      continue;
    }
    strncat(buffer, readbuff, 1);
  } while (readbuff[0] != '\n');
  return 0;
}
