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
#define TWITCH_IRC_IPORT 6667
#define OAUTH "youwish"
#define BOT_NICK "ElNighthawk"

#define MULTI_THREAD false

int start_connection();

void* line_reading_body(void* arg);

void handle_irc_input(const char* buffer, const int blen);

void get_user(const char* buffer, char* dest, const int blen);

void get_message(const char* buffer, char* dest, const int blen);

pthread_t line_reading_thread;
bool running = true;
char line[1100];

int main(void) {
  start_connection();
  send_command("JOIN", "#vavbro");
  if (MULTI_THREAD) {
    int status;
    status = pthread_create(&line_reading_thread, NULL, line_reading_body, NULL);
    printf("Thread status: %d\n", status);
    while (running) {} // Other stuff here.
  } else {
    while (running) {
      memset(line, '\0', 1100);
      read_line(line);
      printf("Line: \"%s\"\n", line);
      handle_irc_input(line, strlen(line));
    }
  }
  printf("Going down...");
  return 0;
}

int start_connection() {
  printf("Starting bot...\n");
  if (connect_to_irc(TWITCH_IRC_ADDRESS, TWITCH_IRC_PORT) != 0) {
    printf("Error experienced while connecting to IRC.");
    return -1;
  }
  send_command("PASS", OAUTH);
  send_command("NICK", BOT_NICK);
  char identity[100];
  memset(identity, '\0', 100);
  strcpy(identity, BOT_NICK);
  strcat(identity, " ");
  strcat(identity, TWITCH_IRC_ADDRESS);
  strcat(identity, " bla :");
  strcat(identity, BOT_NICK);
  send_command("USER", identity);
  return 0;
}

void* line_reading_body(void* arg) {
  while (running) {
    memset(line, '\0', 1100);
    read_line(line);
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
    send_command("PONG", message);
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
    if (starts_with(message, "!", 1)) {
      string_after(message, "!", mlen, 1);
      if (strcmp(user, "vavbro") == 0) {
        if (starts_with(message, "join #", 6)) {
          string_after(message, "join ", mlen, 5);
          send_command("JOIN", message);
        } else if (strcmp(message, "kill") == 0) {
          send_message("#vavbro", "Aye aye, cap'n!");
          running = false;
        }
      }
    }
    return;
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
