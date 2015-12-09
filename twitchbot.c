/*
Todo : Flush
*/

#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "irc_connection.h"

#define TWITCH_IRC_ADDRESS "irc.twitch.tv"
#define TWITCH_IRC_PORT "6667"
#define OAUTH "youwish"
#define BOT_NICK "ElNighthawk"

#define MULTI_THREAD false

int start_connection();

void* line_reading_body(void* arg);

void handle_irc_input(const char* buffer, const int blen);

void handle_command(char* sender, char** args, int ulen, int arglen);

void get_user(const char* buffer, char* dest, const int blen);

void get_message(const char* buffer, char* dest, const int blen);

pthread_t line_reading_thread;
bool running = true;
char line[1100];
struct connection irc_connection;

int main(void) {
  irc_connection.address = TWITCH_IRC_ADDRESS;
  irc_connection.port = TWITCH_IRC_PORT;
  irc_connection.connected = false;
  start_connection(&irc_connection);
  send_command(irc_connection, "JOIN", "#vavbro");
  if (MULTI_THREAD) {
    int status;
    status = pthread_create(&line_reading_thread, NULL, line_reading_body, NULL);
    printf("Thread status: %d\n", status);
    while (running) {} // Other stuff here.
  } else {
    while (running) {
      memset(line, '\0', 1100);
      read_line(irc_connection, line);
      printf("Line: \"%s\"\n", line);
      handle_irc_input(line, strlen(line));
    }
  }
  printf("Going down...");
  return 0;
}

int start_connection(struct connection* to_connect) {
  printf("Starting bot...\n");
  if (connect_to_irc(to_connect) != 0) {
    printf("Error experienced while connecting to IRC.");
    running = false;
    return -1;
  }
  send_command(*to_connect, "PASS", OAUTH);
  send_command(*to_connect, "NICK", BOT_NICK);
  char identity[100];
  memset(identity, '\0', 100);
  strcpy(identity, BOT_NICK);
  strcat(identity, " ");
  strcat(identity, TWITCH_IRC_ADDRESS);
  strcat(identity, " bla :");
  strcat(identity, BOT_NICK);
  send_command(*to_connect, "USER", identity);
  return 0;
}

void* line_reading_body(void* arg) {
  while (running) {
    memset(line, '\0', 1100);
    read_line(irc_connection, line);
    printf("Line: \"%s\"\n", line);
    handle_irc_input(line, strlen(line));
  }
}

void handle_irc_input(const char* buffer, const int blen) {
  if (contains(buffer, "PING ", blen, 5) && !contains(buffer, "PRIVMSG", blen, 7)) {
    char message[1000];
    memset(message, '\0', 1000);
    strcpy(message, buffer);
    string_after(message, "PING ", blen, 5);
    send_command(irc_connection, "PONG", message);
    return;
  }
  if (contains(buffer, "PRIVMSG #", blen, 9)) {
    char user[26];
    char message[1000];
    memset(user, '\0', 26);
    memset(message, '\0', 1000);
    get_user(buffer, user, blen);
    get_message(buffer, message, blen);
    int mlen = strlen(message);
    int ulen = strlen(user);
    if (starts_with(message, "!", mlen, 1)) {
      string_after(message, "!", mlen, 1);
      char** args = split_string(message, " ", mlen, 1);
      int arglen = pattern_count(message, " ", mlen, 1) + 1;
      handle_command(user, args, ulen, arglen);
    }
    return;
  }
}

void handle_command(char* user, char** args, int ulen, int arglen) {
  if (strcmp(user, "vavbro") == 0) {
    switch (arglen) {
      case 1: // 1 Argument commands
        if (strcmp(args[0], "kill") == 0) {
          send_message(irc_connection, "#vavbro", "Aye aye, captain!");
          running = false;
        }
      break;
      case 2: // 2 Argument commands
        if (strcmp(args[0], "join") == 0) {
          send_message(irc_connection, "#vavbro", "Aye aye, captain!");
          send_command(irc_connection, "JOIN", args[1]);
        }
      break;
    }
  }
}

//There's a flaw where sometimes there'll be a random char at end of string.
void get_user(const char* buffer, char* dest, const int blen) {
  char user[26];
  memset(user, '\0', 26); // Max twitch username length + 1 for nullchar
  int i;
  int idx = 0;
  for (i = 0; i < blen; i++) {
    if (buffer[i] != ':') {
      continue;
    }
    i++;
    do {
      user[idx++] = buffer[i++];
    } while (buffer[i] != '!');
    strcpy(dest, user);
    return;
  }
}

//There's a flaw where sometimes there'll be a random char at end of string.
void get_message(const char* buffer, char* dest, const int blen) {
  char message[1000]; // 500 is the highest I can possibly convieve every happening.
  memset(message, '\0', 1000);
  int i;
  int idx = 0;
  bool msg = false;
  for (i = 39; i < blen; i++) { // Skipping ahead the minimum amount to get to "#"
    if (msg) {
      message[idx++] = buffer[i];
      continue;
    }
    if (buffer[i] == ':') {
      msg = true;
      continue;
    }
  }
  if (msg) {
    strcpy(dest, message);
  }
}
