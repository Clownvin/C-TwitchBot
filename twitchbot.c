/*
Todo : Fix mo memory leaks, FIND OUT WHICH FREE IS CAUSING EXIT


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
#define TEST_STRING "test"

int start_connection();

void* line_reading_body(void* arg);

void handle_irc_input(const char* buffer);

void string_after(char* buffer, const char* match);

bool contains(const char* buffer, const char* match);

void get_user(const char* buffer, char* dest);

void get_message(const char* buffer, char* dest);

pthread_t line_reading_thread;
bool running = true;
char line[1100];

int main(void) {
  start_connection();
  send_command("JOIN", "#dendi");
  send_command("JOIN", "#domingo");
  send_command("JOIN", "#lirik");
  send_command("JOIN", "#rocketbeanstv");
  send_command("JOIN", "#dreadztv");
  send_command("JOIN", "#starladder5");
  send_command("JOIN", "#sodapoppin");
  send_command("JOIN", "#lainkk");
  send_command("JOIN", "#kungentv");
  send_command("JOIN", "#wagamamatv");
  send_command("JOIN", "#yogscast");
  send_command("JOIN", "#starladder_hs_en");
  send_command("JOIN", "#forsenlol");
  send_command("JOIN", "#faceittv");
  send_command("JOIN", "#esl_greatfrag");
  send_command("JOIN", "#imaqtpie");
  if (MULTI_THREAD) {
    int status;
    status = pthread_create(&line_reading_thread, NULL, line_reading_body, NULL);
    printf("Thread status: %d\n", status);
    while (running) {} // Other stuff here.
  } else {
    while (running) {
      strclear(line);
      read_line(line);
      printf("Line: \"%s\"\n", line);
      handle_irc_input(line);
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
  strclear(identity);
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
    strclear(line);
    read_line(line);
    printf("Line: \"%s\"\n", line);
    handle_irc_input(line);
  }
}

void handle_irc_input(const char* buffer) {
  if (contains(buffer, "PING") && !contains(buffer, "PRIVMSG")) {
    char message[1000];
    strclear(message);
    strcpy(message, buffer);
    string_after(message, "PING ");
    send_command("PONG", message);
    return;
  }
  if (contains(buffer, "PRIVMSG #")) {
    char user[26];
    char message[1000];
    strclear(user);
    strclear(message);
    get_user(buffer, user);
    get_message(buffer, message);
    if (strcmp(user, "vavbro") == 0) {
      if (strcmp(message, "kill") == 0) {
        send_message("#vavbro", "Aye aye, cap'n!");
        running = false;
      }
      if (strcmp(message, TEST_STRING) == 0) {
        send_message("#vavbro", "Arrrr, she be workin!");
      }
    }
    return;
  }
}

void string_after(char* buffer, const char* match) {
  int blen = strlen(buffer);
  int mlen = strlen(match);
  if (mlen < 1 || blen < 1 || blen <= mlen) {
    return;
  }
  int i;
  int j;
  for (i = 0; i < blen - mlen; i++) {
    bool matches = true;
    for (j = 0; j < mlen; j++) {
      if (buffer[i + j] != match[j]) {
        matches = false;
        break;
      }
    }
    if (matches) { // now i + mlen = start
      char* newstr = malloc((blen - (i + mlen) + 1) * sizeof(char));
      for (i = i + mlen, j = 0; i < blen; i++, j++) {
        newstr[j] = buffer[i];
      }
      strcpy(buffer, newstr);
      free (newstr);
      return;
    }
  }
  return;
}

bool contains(const char* buffer, const char* match) {
  int blen = strlen(buffer);
  int mlen = strlen(match);
  if (mlen < 1 || blen < 1 || blen < mlen) {
    return false;
  }
  int i;
  int j;
  for (i = 0; i < blen - mlen; i++) {
    bool matches = true;
    for (j = 0; j < mlen; j++) {
      if (buffer[i + j] != match[j]) {
        matches = false;
        break;
      }
    }
    if (matches) {
      return true;
    }
  }
  return false;
}

void get_user(const char* buffer, char* dest) {
  char user[26]; // Max twitch username length + 1 for nullchar
  strclear(user);
  int idx = 0;
  int i;
  int blen = strlen(buffer);
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

void get_message(const char* buffer, char* dest) {
  int blen = strlen(buffer);
  char message[1000]; // 500 is the highest I can possibly convieve every happening.
  strclear(message);
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
