#include <stdbool.h>
#include <stdio.h>

void string_after(char* buffer, const char* match, const int blen, const int mlen);

bool contains(const char* buffer, const char* match, const int blen, const int mlen);

bool starts_with(const char* buffer, const char* match, const int mlen);

void split_string(const char* buffer, const char* regex, char** dest, const int blen, const int rlen);

int pattern_count(const char* buffer, const char* regex, const int blen, const int rlen);

void string_after(char* buffer, const char* match, const int blen, const int mlen) {
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
      int len = (blen - (i + mlen) + 1);
      char* newstr = malloc(len);
      memset(newstr, '\0', len);
      for (i = i + mlen, j = 0; i < blen; i++, j++) {
        newstr[j] = buffer[i];
      }
      memset(buffer, '\0', blen);
      strcpy(buffer, newstr);
      free (newstr);
      return;
    }
  }
  return;
}

bool contains(const char* buffer, const char* match, const int blen, const int mlen) {
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

bool starts_with(const char* buffer, const char* match, const int mlen) {
  char* fsequence = malloc(mlen * 2); // *2 because apparently twitch allows 2byte chars.
  strncpy(fsequence, buffer, mlen); // Copy mlen chars into fsequence
  bool val = strcmp(fsequence, match) == 0; // check match
  free (fsequence);
  return val;
}

void split_string(const char* buffer, const char* regex, char** dest, const int blen, const int rlen) {
  int count = pattern_count(buffer, regex, blen, rlen);
  char pre[count][blen];

}

int pattern_count(const char* buffer, const char* regex, const int blen, const int rlen) {
  int i, j;
  int count = 0;
  for (i = 0; i < blen - rlen; i++) {
    bool match = true;
    for (j = 0; j < rlen; j++) {
      if (buffer[i + j] != regex[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      count++;
      i += rlen; // Skip ahead
    }
  }
  return count;
}
