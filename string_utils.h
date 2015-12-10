#include <stdbool.h>
#include <stdio.h>

void string_after(char* buffer, const char* match, const int blen, const int mlen);

bool contains(const char* buffer, const char* match, const int blen, const int mlen);

bool starts_with(const char* buffer, const char* match, const int blen, const int mlen);

char** split_string(const char* buffer, const char* regex, const int blen, const int rlen);

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

bool starts_with(const char* buffer, const char* match, const int blen, const int mlen) {
  if (blen < mlen) {
    return false;
  }
  int i;
  bool matches = true;
  for (i = 0; i < mlen; i++) {
    if (buffer[i] != match[i]) {
      matches = false;
      break;
    }
  }
  return matches;
}

char** split_string(const char* buffer, const char* regex, const int blen, const int rlen) {
  int count = pattern_count(buffer, regex, blen, rlen) + 1;
  char** pre = calloc(count, blen);
  int idx1 = 0, idx2 = 0, j, k;
  for (j = 0; j < count; j++) {
    pre[j] = (char*)malloc(blen);
    memset(pre[j], '\0', blen);
  }
  if (count == 1) {
    strcpy(pre[0], buffer);
    return pre;
  }
  for (j = 0; j < blen; j++) {
    if (buffer[j] == regex[0]) {
      bool match = true;
      for (k = j + 1; k < j + rlen; k++) {
        if (buffer[k] != regex[k - j]) {
          match = false;
          break;
        }
      }
      if (match) {
        pre[idx1++][idx2] = '\0'; // Close off string
        idx2 = 0;
        j += rlen - 1; // Skip ahead
        continue;
      }
    }
    pre[idx1][idx2++] = buffer[j];
  }
  pre[idx1][idx2 + 1] = '\0'; // Close off final string.
  return pre;
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
