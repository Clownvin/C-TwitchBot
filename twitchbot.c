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

//Constants
#define MAX_IRC_MESSAGE_LEN 1100
#define MAX_TWITCH_MESSAGE_LEN 1000 // 500*2 to protect against extended ASCII chars
#define MAX_TWITCH_USER_LEN 26
#define CONNECT_RETRIES 10 // Per connection. Program will continue even when a connection goes silent.
//IRC Info
#define TWITCH_IRC_ADDRESS "irc.twitch.tv"
#define TWITCH_IRC_PORT "6667"
//Group IRC Info
#define GROUP_IRC_ADDRESS "192.16.64.180"
#define GROUP_IRC_PORT "443"
//Bot Info
#define OAUTH "you wish"
#define BOT_NICK "ElNighthawk"
#define DEFAULT_CHANNEL "#vavbro"

#define MULTI_THREAD true

int start_connection();

void* whisper_reading_body(void* arg); // For reading whispers

void* raw_input_body(void* arg);

void handle_irc_input(const char* buffer, const int blen);

void handle_command(char* sender, char** args, int ulen, int arglen);

void get_user(const char* buffer, char* dest, const int blen);

void get_message(const char* buffer, char* dest, const int blen);

pthread_t whisper_reading_thread;
pthread_t raw_input_thread;
bool running = true;
char line[MAX_IRC_MESSAGE_LEN];
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
  send_command(irc_connection, "JOIN", DEFAULT_CHANNEL);
  if (MULTI_THREAD) {
    start_connection(&group_connection); // Start group inside multi_thread block, since we can't use it if not multi-thread
    send_command(group_connection, "CAP REQ", ":twitch.tv/commands");
    int status;
    status = pthread_create(&whisper_reading_thread, NULL, whisper_reading_body, NULL);
    if (status != 0) {
      printf("GROUP thread status: %d\n", status);
    }
    status = pthread_create(&raw_input_thread, NULL, raw_input_body, NULL);
    if (status != 0) {
      printf("RAW INPUT thread status: %d\n", status);
    }
  }
  while (running) {
    memset(line, '\0', MAX_IRC_MESSAGE_LEN);
    if (read_line(irc_connection, line) != 0) {
      close_connection(&irc_connection);
      int retries_left = CONNECT_RETRIES;
      printf("Error: Seems irc connection is down. Retrying... \n");
      while (!irc_connection.connected && retries_left > 0) {
        sleep(30);
        connect_to_irc(&irc_connection);
        retries_left--;
      }
      if (!group_connection.connected) {
        printf("Failed to reconnect to irc server.. exiting program.\n");
        return -1;
      }
      continue;
    }
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
  char whisper_line[MAX_IRC_MESSAGE_LEN];
  while (running) {
    memset(whisper_line, '\0', MAX_IRC_MESSAGE_LEN);
    if (read_line(group_connection, whisper_line) != 0) {
      close_connection(&group_connection);
      int retries_left = CONNECT_RETRIES;
      printf("Error: Seems group connection is down. Retrying... \n");
      while (!group_connection.connected && retries_left > 0) {
        sleep(30);
        connect_to_irc(&group_connection);
        retries_left--;
      }
      if (!group_connection.connected) {
        printf("Failed to reconnect to group server.. exiting whisper reading thread.\n");
        break;
      }
      continue;
    }
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

void* raw_input_body(void* arg) {
  char raw_input_line[500];
  while (running) {
    memset(raw_input_line, '\0', 500);
    scanf(" %[^\n]", raw_input_line);
    char channel[MAX_TWITCH_USER_LEN+1] = DEFAULT_CHANNEL;
    int rlen = strlen(raw_input_line);
    if (starts_with(raw_input_line, "#", rlen, 1)) {
      int i;
      for (i = 0; i < MAX_TWITCH_USER_LEN; i++) {
        if (raw_input_line[i] == ' ') {
          channel[i] = '\0'; // End string
          break;
        }
        channel[i] = raw_input_line[i];
      }
    }
    if (starts_with(raw_input_line, "/w", rlen, 2)) {
      send_message(group_connection, channel, raw_input_line);
      continue;
    }
    send_message(irc_connection, channel, raw_input_line);
  }
}

void handle_irc_input(const char* buffer, const int blen) {
  if (!contains(buffer, " PRIVMSG #", blen, 10) && !contains(buffer, " WHISPER ", blen, 9)) {
    return;
  }
  char user[MAX_TWITCH_USER_LEN];
  char message[MAX_TWITCH_MESSAGE_LEN];
  memset(user, '\0', MAX_TWITCH_USER_LEN);
  memset(message, '\0', MAX_TWITCH_MESSAGE_LEN);
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
          send_message(irc_connection, DEFAULT_CHANNEL, "Aye aye, captain!");
          running = false;
        } else if (strcmp(args[0], "test") == 0) {
          send_message(irc_connection, DEFAULT_CHANNEL, "Test recieved, captain!");
          send_whisper(group_connection, DEFAULT_CHANNEL, user, "Test recieved, captain!");
        }
      break;
      case 2: // 2 Argument commands
        if (strcmp(args[0], "join") == 0) {
          send_message(irc_connection, DEFAULT_CHANNEL, "Aye aye, captain!");
          send_command(irc_connection, "JOIN", args[1]);
        }
      break;
    }
  }
}

void get_user(const char* buffer, char* dest, const int blen) {
  char user[MAX_TWITCH_USER_LEN];
  memset(user, '\0', MAX_TWITCH_USER_LEN); // Max twitch username length + 1 for nullchar
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

void get_message(const char* buffer, char* dest, const int blen) {
  char message[MAX_TWITCH_MESSAGE_LEN]; // 500 is maximum. Using 1000 because Twitch allows chars with len 2 instead of 1
  memset(message, '\0', MAX_TWITCH_MESSAGE_LEN);
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
