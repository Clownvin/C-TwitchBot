#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

int status = 0;
int socketfd; // The socket descripter
bool connected = false;

struct addrinfo host_info; // Host info
struct addrinfo *host_info_list; // Host info list

int connect_to_irc(const char * address, const char * port); // Returns status

int send_line(char * buffer);

int send_command(char * command, char * message);

int send_message(char * channel, char * message);

int send_whisper(char * channel, char * user, char * message);

void strclear(char * buffer);

int read_line(char * buffer);

int connect_to_irc(const char * address, const char * port) {
  memset(&host_info, 0, sizeof host_info);

  host_info.ai_family = AF_UNSPEC; // IP Version not specified.
  host_info.ai_socktype = SOCK_STREAM; // SOCK_STREAM for TCP, SOCK_DGRAM for UDP

  getaddrinfo(address, port, &host_info, &host_info_list);
  if (status != 0) {
    printf("getaddrinfo error: %s\n", gai_strerror(status));
    return status;
  }

  printf("Creating a socket...\n");
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
  if (socketfd == -1) {
    printf("socket error\n");
    return -1;
  }
  printf("Connecting...\n");
  status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    printf("connect error.\n");
    return status;
  }
  connected = true;
  return 0;
}

int send_line(char * buffer) {
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
  bytes_sent = send(socketfd, buffer, len, 0);
  return bytes_sent == len ? 0 : -1;
}

int send_command(char * command, char * message) {
  char combined[500];
  strclear(combined);
  strcpy(combined, command);
  strcat(combined, " ");
  strcat(combined, message);
  strcat(combined, "\r\n");
  send_line(combined);
}

int send_message(char * channel, char * message) {
  char combined[500];
  strclear(combined);
  strcpy(combined, "PRIVMSG ");
  strcat(combined, channel);
  strcat(combined, " :");
  strcat(combined, message);
  strcat(combined, "\r\n");
  send_line(combined);
}

int send_whisper(char * channel, char * user, char * message) {
  char combined[500];
  strclear(combined);
  strcpy(combined, "/w ");
  strcat(combined, user);
  strcat(combined, " ");
  strcat(combined, message);
  send_message(channel, combined);
}

void strclear(char * buffer) {
  int blen = strlen(buffer);
  int i;
  for (i = 0; i < blen; i++) {
    buffer[i] = '\0';
  }
}

int read_line(char * buffer) {
  if (!connected) {
    printf("Socket not connected.\n");
    return 1;
  }
  char readbuff[1]; // 1 byte of read.
  ssize_t bytes_recieved;
  strcpy(buffer, "");
  do {
    bytes_recieved = recv(socketfd, readbuff, 1, 0);
    if (bytes_recieved == 0) {
      printf("host shut down.\n");
      return 2;
    }
    if (bytes_recieved == -1) {
      printf("read error\n");
      return 3;
    }
    if (readbuff[0] == '\r' || readbuff[0] == '\n') {
      continue;
    }
    strncat(buffer, readbuff, 1);
  } while (readbuff[0] != '\n');
  return 0;
}
