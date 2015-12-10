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

//IRC Info
#define TWITCH_IRC_ADDRESS "irc.twitch.tv"
#define TWITCH_IRC_PORT "6667"
//Group IRC Info
#define GROUP_IRC_ADDRESS "192.16.64.180"
#define GROUP_IRC_PORT "443"
//Bot Info
#define OAUTH "you wish"
#define BOT_NICK "ElNighthawk"

#define MULTI_THREAD true

int start_connection();

void* whisper_reading_body(void* arg); // For reading whispers

void handle_irc_input(const char* buffer, const int blen);

void handle_command(char* sender, char** args, int ulen, int arglen);

void get_user(const char* buffer, char* dest, const int blen);

void get_message(const char* buffer, char* dest, const int blen);

pthread_t whisper_reading_thread;
bool running = true;
char line[1100];
struct connection irc_connection;
struct connection group_connection;

int main(void) {
  //Store IRC info
  irc_connection.address = TWITCH_IRC_ADDRESS;
  irc_connection.port = TWITCH_IRC_PORT;
  irc_connection.connected = false;
  //Store group info
  group_connection.address = GROUP_IRC_ADDRESS;
  group_connection.port = GROUP_IRC_PORT;
  group_connection.connected = false;

  start_connection(&irc_connection);
  send_command(irc_connection, "JOIN", "#vavbro");
  if (MULTI_THREAD) {
    start_connection(&group_connection); // Start group inside multi_thread block, since we can't use it if not multi-thread
    send_command(group_connection, "CAP REQ", ":twitch.tv/commands");
    int status;
    status = pthread_create(&whisper_reading_thread, NULL, whisper_reading_body, NULL);
    printf("GROUP thread status: %d\n", status);
  }
  while (running) {
    memset(line, '\0', 1100);
    read_line(irc_connection, line);
    printf("IRC: \"%s\"\n", line);
    int len = strlen(line);
    if (contains(line, "PING :", len, 6) && !contains(line, "PRIVMSG", len, 7)) {
      char message[100];
      memset(message, '\0', 100);
      strcpy(message, line);
      string_after(message, "PING ", len, 5);
      send_command(irc_connection, "PONG", message);
      continue;
    }
    handle_irc_input(line, len);
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

void* whisper_reading_body(void* arg) {
  char whisper_line[1100];
  while (running) {
    memset(whisper_line, '\0', 1100);
    read_line(group_connection, whisper_line);
    printf("GROUP: \"%s\"\n", whisper_line);
    int wlen = strlen(whisper_line);
    if (contains(whisper_line, "PING :", wlen, 6) && !contains(whisper_line, "PRIVMSG", wlen, 7)) {
      char message[100];
      memset(message, '\0', 100);
      strcpy(message, whisper_line);
      string_after(message, "PING ", wlen, 5);
      send_command(group_connection, "PONG", message);
      continue;
    }
    handle_irc_input(whisper_line, wlen);
  }
}

void handle_irc_input(const char* buffer, const int blen) {
  if (!contains(buffer, " PRIVMSG #", blen, 10) && !contains(buffer, " WHISPER ", blen, 9)) {
    return;
  }
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
    free(args);
  }
}

void handle_command(char* user, char** args, int ulen, int arglen) {
  if (strcmp(user, "vavbro") == 0) {
    switch (arglen) {
      case 1: // 1 Argument commands
        if (strcmp(args[0], "kill") == 0) {
          send_message(irc_connection, "#vavbro", "Aye aye, captain!");
          running = false;
        } else if (strcmp(args[0], "test") == 0) {
          send_message(irc_connection, "#vavbro", "Test recieved, captain!");
          send_whisper(group_connection, "#vavbro", user, "Test recieved, captain!");
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
  char message[1000]; // 500 is maximum. Using 1000 because Twitch allows chars with len 2 instead of 1
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
