#include "shlib.h"
#include "operationstate.h"

#define DEFALUT_LINE_SIZE         512

// 32 should be enough... for use
#define DEFALUT_VARIABLE_NUM      32
#define DEFALUT_VARIABLE_NAME     64
#define DEFALUT_VARIABLE_CONTENT  512

#define CONDITION_CHOSER          15
#define ACCEPTED_CHOSER           16

#define IF_SIGN                   1
#define FI_SIGN                   2
#define ELSE_SIGN                 3
#define ELIF_SIGN                 4
#define THEN_SIGN                 5

#define FOR_SIGN                  6
#define DO_SIGN                   7
#define DONE_SIGN                 8

#define WHILE_SIGN                9
#define UNTIL_SIGN                10

#define CONDITION_ACCEPTED        16
#define CONDITION_REJECTED        0

int fd = -1, i, j, n, parsedollarindex, mcmdindex;
char buf[512], ans[512], mcmd[2048], name[DEFALUT_VARIABLE_NAME];
int quoteSign = ' ';

struct variable {
  char name[DEFALUT_VARIABLE_NAME];
  char value[DEFALUT_VARIABLE_CONTENT];
};
struct variable vs[DEFALUT_VARIABLE_NUM];
int vlen = 0;
int vsize = DEFALUT_VARIABLE_NUM;

// 32 bit; last 4 bit for SIGN defines, last 5th bit for condition accepted or rejected;
// (conditionlist[i] << 13) >> 19 for start index of conditionStatements
// conditionlist[i] >> 19 for end index of conditionStatements(not included)
int conditionlist[128];
int conditionlistindex = 0;

// for for, while, until loop
char conditionStatements[512][512];
int conditionStatementsIndex = 0;

char mshErrorInfo[512];

int whilenextreadline[128];
int whilenextreadlineindex = 0;

void strGetvariable(char* temp, int* index) {
  int tempIndex;
  for (tempIndex = 0; tempIndex < vlen; tempIndex++) {
    if (strcmp(name, vs[tempIndex].name) == 0) {
      if (*index + strlen(vs[tempIndex].value) >= 512 - 1) {
        return;
      }
      strcpy(temp + *index, vs[tempIndex].value);
      *index += strlen(vs[tempIndex].value);
      return;
    }
  }
  if (tempIndex == vlen) {
    // Find in sysenv;
    struct env m;
    int sysenvnum = getenv(1, &m, name);
    if (sysenvnum == 0 && m.len > 0) {
      // Found!
      int variablesTotalLen = -1;
      for (uint i = 0; i < m.len; i++) {
        variablesTotalLen += (1 + strlen(m.text[i]));
      }
      if (*index + variablesTotalLen >= 512 - 1) {
        return;
      }
      strcpy(temp + *index, m.text[0]);
      *index += strlen(m.text[0]);
      for (int j = 0; j < m.len; j++) {
        strcpy(temp + *index, ":");
        *index += 1;
        strcpy(temp + *index, m.text[j]);
        *index += strlen(m.text[j]);
      }
    }
  }
  return;
}

STATE getvariable() {
  int tempIndex;
  for (tempIndex = 0; tempIndex < vlen; tempIndex++) {
    if (strcmp(name, vs[tempIndex].name) == 0) {
      if (mcmdindex + strlen(vs[tempIndex].value) >= 2048 - 1) {
        strcpy(mshErrorInfo, "Can't replace $ variables since that command is too long after replacing $!");
        return STATE_ERROR;
      }
      strcpy(mcmd + mcmdindex, vs[tempIndex].value);
      mcmdindex += strlen(vs[tempIndex].value);
      return STATE_OK;
    }
  }
  if (tempIndex == vlen) {
    // Find in sysenv;
    struct env m;
    int sysenvnum = getenv(1, &m, name);
    if (sysenvnum == 0 && m.len > 0) {
      // Found!
      int variablesTotalLen = -1;
      for (uint i = 0; i < m.len; i++) {
        variablesTotalLen += (1 + strlen(m.text[i]));
      }
      if (mcmdindex + variablesTotalLen >= 2048 - 1) {
        strcpy(mshErrorInfo, "Can't replace $ variables since that command is too long after replacing $!");
        return STATE_ERROR;
      }
      strcpy(mcmd + mcmdindex, m.text[0]);
      mcmdindex += strlen(m.text[0]);
      for (int j = 0; j < m.len; j++) {
        strcpy(mcmd + mcmdindex, ":");
        mcmdindex += 1;
        strcpy(mcmd + mcmdindex, m.text[j]);
        mcmdindex += strlen(m.text[j]);
      }
    }
  }
  return STATE_OK;
}

int subString(char* ori, char* substring) {
  int lenori = strlen(ori);
  int lensub = strlen(substring);
  for (int i = 0; i <= lenori - lensub; i++) {
    int j = 0;
    for (; j < lensub && substring[j] == ori[i + j]; j++);
    if (j == lensub) {
      return i;
    }
  }
  return -1;
}

int strcmpParsingDollar(char* a, char* b) {
  char aWithoutDollar[512], bWithoutDollar[512];
  int index = 0, alen = strlen(a);
  int blen = strlen(b);
  for (int i = 0; i < alen; i++) {
    if (a[i] == '$') {
      // Get the word;
      int nameindex = 0;
      for (i++; ; i++) {
        if (!a[i] || strchr(" \t\r\n\v\"$", a[i])) {
          name[nameindex++] = 0;
          i--;
          break;
        }
        name[nameindex++] = a[i];
        if (nameindex >= DEFALUT_VARIABLE_NAME) {
          goto FINALCMP;
        }
      }
      // Get var;
      strGetvariable(aWithoutDollar, &index);
    }
    else {
      aWithoutDollar[index++] = a[i];
    }
  }
  aWithoutDollar[index] = 0;

  index = 0;
  for (int i = 0; i < blen; i++) {
    if (b[i] == '$') {
      // Get the word;
      int nameindex = 0;
      for (i++; ; i++) {
        if (!b[i] || strchr(" \t\r\n\v\"$", b[i])) {
          name[nameindex++] = 0;
          i--;
          break;
        }
        name[nameindex++] = b[i];
        if (nameindex >= DEFALUT_VARIABLE_NAME) {
          goto FINALCMP;
        }
      }
      // Get var;
      strGetvariable(bWithoutDollar, &index);
    }
    else {
      bWithoutDollar[index++] = b[i];
    }
  }
  bWithoutDollar[index] = 0;
FINALCMP:
  return strcmp(aWithoutDollar, bWithoutDollar);
}

// Only return 0 or 1;
int parseCondition(char* condition) {
  // [] or [[]]
  if (condition[0] == '[' && condition[strlen(condition) - 1] == ']') {
    condition[strlen(condition) - 1] = 0;
    return parseCondition(condition + 1);
  }
  if (condition[0] == '!') {
    return 1 - parseCondition(condition + 1);
  }
  int index;
  index = subString(condition, "&&");
  if (index < 0) {
    index = subString(condition, "-a");
  }
  if (index >= 0) {
    condition[index] = 0;
    return parseCondition(condition) && parseCondition(condition + index + 2);
  }
  index = subString(condition, "||");
  if (index < 0) {
    index = subString(condition, "-o");
  }
  if (index >= 0) {
    condition[index] = 0;
    return parseCondition(condition) || parseCondition(condition + index + 2);
  }
  int operationlen = 2;
  index = subString(condition, "!=");
  if (index < 0) {
    index = subString(condition, "-ne");
    operationlen = 3;
  }
  if (index >= 0) {
    condition[index] = 0;
    return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen));
  }
  index = subString(condition, ">=");
  operationlen = 2;
  if (index >= 0) {
    condition[index] = 0;
    return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen)) >= 0;
  }
  index = subString(condition, "<=");
  operationlen = 2;
  if (index >= 0) {
    condition[index] = 0;
    return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen)) <= 0;
  }
  index = subString(condition, "=");
  if (index >= 0) {
    condition[index] = 0;
    return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + 1 + (condition[index + 1] == '='))) == 0;
  }
  index = subString(condition, ">");
  operationlen = 1;
  if (index >= 0) {
    condition[index] = 0;
    return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen)) > 0;
  }
  index = subString(condition, "<");
  operationlen = 1;
  if (index >= 0) {
    condition[index] = 0;
    return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen)) < 0;
  }
  return 0;
}

STATE setVariables(char* name, char* value) {
  for (int i = 0; i < vlen; i++) {
    if (strcmp(name, vs[i].name) == 0) {
      if (strlen(value) >= DEFALUT_VARIABLE_CONTENT) {
        strcpy(mshErrorInfo, "Can't set variable since that the value is too long!");
        return STATE_OVERFLOW;
      }
      strcpy(vs[i].value, value);
      return STATE_OK;
    }
  }
  if (vlen == vsize) {
    strcpy(mshErrorInfo, "No space left for variables!");
    return STATE_OVERFLOW;
  }
  if (strlen(name) >= DEFALUT_VARIABLE_NAME) {
    strcpy(mshErrorInfo, "Can't set variable since that the name is too long!");
    return STATE_OVERFLOW;
  }
  strcpy(vs[vlen].name, name);
  strcpy(vs[vlen].value, value);
  vlen++;
  return STATE_OK;
}

int loadCommand() {
  if (whilenextreadlineindex > 0) {
    strcpy(ans, conditionStatements[whilenextreadline[whilenextreadlineindex - 1]++]);
    return 1;
  }
  while (1) {
    if (i == n){
      n = read(fd, buf, sizeof(buf));
      i = 0;
    }
    if (n > 0) {
      while (i < n) {
        char temp = buf[i++];
        if (temp == '\'') {
          if (quoteSign == '\'') {
            quoteSign = ' ';
          }
          else if (quoteSign == ' ') {
            quoteSign = '\'';
          }
        }
        else if (temp == '\"') {
          if (quoteSign == '\"') {
            quoteSign = ' ';
          }
          else if (quoteSign == ' ') {
            quoteSign = '\"';
          }
        }
        if (temp == '\n') {
          if (j >= 512) {
            return 0;
          }
          ans[j] = 0;
          if (quoteSign == ' ') {
            if (j > 0) {
              j = 0;
              return 1;
            }
            continue;
          }
        }
        ans[j++] = temp;
        if (j >= 512) {
          return 0;
        }
      }
    }
    else {
      if (j >= 512) {
        return 0;
      }
      if (j == 0) {
        return 0;
      }
      ans[j] = 0;
      if (j > 0) {
        j = 0;
        return 1;
      }
    }
  }
  return 1;
}

STATE parsedollar() {
  int quote = ' ', anslen = strlen(ans);
  parsedollarindex = strimAndTrip(ans) - ans;
  if (strprefix(ans + parsedollarindex, "if ") || strprefix(ans + parsedollarindex, "while ") || strprefix(ans + parsedollarindex, "elif ")) {
    strcpy(mcmd, ans + parsedollarindex);
    return STATE_OK;
  }
  mcmdindex = 0;
  for (; parsedollarindex < anslen; parsedollarindex++) {
    switch (ans[parsedollarindex]) {
      case '\'':
        if (quote == ' ') {
          quote = '\'';
          break;
        }
        else if (quote == '\'') {
          quote = ' ';
          break;
        }
      case '\"':
        if (quote == ' ') {
          quote = '\"';
          break;
        }
        else if (quote == '\"') {
          quote = ' ';
          break;
        }
      case '$':
      if (quote == '\"' || quote == ' ') {
        // Get the word;
        int nameindex = 0;
        for (parsedollarindex++; ; parsedollarindex++) {
          if (!ans[parsedollarindex] || strchr(" \t\r\n\v\"$", ans[parsedollarindex])) {
            name[nameindex++] = 0;
            parsedollarindex --;
            break;
          }
          name[nameindex++] = ans[parsedollarindex];
          if (nameindex >= DEFALUT_VARIABLE_NAME) {
            strcpy(mshErrorInfo, "Can't replace $ variables since that variable name is too long!");
            return STATE_ERROR;
          }
        }
        if (getvariable() == STATE_ERROR) {
          strcpy(mshErrorInfo, "Can't replace $ variables since that error occured when try to get variable content!");
          return STATE_ERROR;
        }
        break;
      }
      default:
        mcmd[mcmdindex++] = ans[parsedollarindex];
        if (mcmdindex >= 2048) {
          strcpy(mshErrorInfo, "Can't replace $ variables since that command is too long after replacing $!");
          return STATE_ERROR;
        }
    }
  }
  mcmd[mcmdindex] = 0;
  return STATE_OK;
}

STATE mruncmd() {
  int run = mcmd[0];
  if (run == '#') {
    run = 0;
  }
  else {
    if (conditionlistindex > 0 && (conditionlist[conditionlistindex - 1] & ACCEPTED_CHOSER) == 0) {
      if (strprefix(mcmd, "if ")) {
        // Find corresponding fi
        int ifcount = 1;
        while (ifcount > 0 && loadCommand() && parsedollar() == STATE_OK) {
          if (strprefix(mcmd, "if ")) {
            ifcount++;
          }
          if (strcmp(mcmd, "fi") == 0) {
            ifcount--;
            if (ifcount == 0) {
              break;
              return STATE_OK;
            }
          }
        }
        strcpy(mshErrorInfo, "Parsing error! Didn't find corresponding fi!");
        return STATE_ERROR;
      }
      if (strprefix(mcmd, "while ")) {
        int whilecount = 1;
        while (whilecount > 0 && loadCommand() && parsedollar() == STATE_OK) {
          if (strprefix(mcmd, "while ")) {
            whilecount++;
          }
          if (strcmp(mcmd, "done") == 0) {
            whilecount--;
            if (whilecount == 0) {
              break;
              return STATE_OK;
            }
          }
        }
      }
    }
    if (strprefix(mcmd, "if ")) {
      if (conditionlistindex >= 128) {
        strcpy(mshErrorInfo, "Too many if conditions, while loops!");
        return STATE_OVERFLOW;
      }
      int parseif = parseCondition(mcmd + 3);
      conditionlist[conditionlistindex++] = parseif ? (IF_SIGN | CONDITION_ACCEPTED) : (IF_SIGN | CONDITION_REJECTED);
      return STATE_OK;
    }
    else if (strcmp(mcmd, "then") == 0) {
      return STATE_OK;
    }
    else if (strprefix(mcmd, "elif ")) {
      if (conditionlistindex >= 128) {
        strcpy(mshErrorInfo, "Too many if conditions, while loops!");
        return STATE_OVERFLOW;
      }
      if (conditionlistindex < 1 || (((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != IF_SIGN) && ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != ELIF_SIGN))) {
        strcpy(mshErrorInfo, "Parsing error! Can't accept elif!");
        return STATE_ERROR;
      }
      int parseelif = 1;
      for (int tempi = 0; tempi < conditionlistindex; tempi++) {
        if (conditionlist[conditionlistindex - 1 - tempi] & ACCEPTED_CHOSER) {
          parseelif = 0;
          break;
        }
        if ((conditionlist[conditionlistindex - 1 - tempi] & CONDITION_CHOSER) == IF_SIGN) {
          break;
        }
      }
      if (parseelif && parseCondition(mcmd + 5)) {
        parseelif = 1;
      }
      else {
        parseelif = 0;
      }
      conditionlist[conditionlistindex++] = parseelif ? (ELIF_SIGN | CONDITION_ACCEPTED) : (ELIF_SIGN | CONDITION_REJECTED);
      return STATE_OK;
    }
    else if (strcmp(mcmd, "else") == 0) {
      if (conditionlistindex >= 128) {
        strcpy(mshErrorInfo, "Too many if conditions, while loops!");
        return STATE_OVERFLOW;
      }
      if (conditionlistindex < 1 || (((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != IF_SIGN) && ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != ELIF_SIGN))) {
        strcpy(mshErrorInfo, "Parsing error! Can't accept else!");
        return STATE_ERROR;
      }
      int parseelse = 1;
      for (int tempi = 0; tempi < conditionlistindex; tempi++) {
        if (conditionlist[conditionlistindex - 1 - tempi] & ACCEPTED_CHOSER) {
          parseelse = 0;
          break;
        }
        if ((conditionlist[conditionlistindex - 1 - tempi] & CONDITION_CHOSER) == IF_SIGN) {
          break;
        }
      }
      conditionlist[conditionlistindex++] = parseelse ? (ELSE_SIGN | CONDITION_ACCEPTED) : (ELSE_SIGN | CONDITION_REJECTED);
      return STATE_OK;
    }
    else if (strcmp(mcmd, "fi") == 0) {
      if (conditionlistindex < 1 || (((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != IF_SIGN) && ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != ELIF_SIGN) && ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != ELSE_SIGN))) {
        strcpy(mshErrorInfo, "Parsing error! Can't accept fi!");
        return STATE_ERROR;
      }
      while (conditionlistindex > 0) {
        if ((conditionlist[--conditionlistindex] & CONDITION_CHOSER) == IF_SIGN) {
          break;
        }
      }
      return STATE_OK;
    }
    else if (strprefix(mcmd, "while ")) {
      if (conditionlistindex >= 128) {
        strcpy(mshErrorInfo, "Too many if conditions, while loops!");
        return STATE_OVERFLOW;
      }
      char whilecondition[512];
      strcpy(whilecondition, mcmd + 6);
      char tempwhilecondition[512];
      strcpy(tempwhilecondition, whilecondition);
      int parsewhile = parseCondition(tempwhilecondition);
      // load all the lines...
      if (loadCommand() == 0 || parsedollar() != STATE_OK || strcmp(mcmd, "do")) {
        strcpy(mshErrorInfo, "Parsing error! Can't accept while!");
        return STATE_ERROR;
      }
      int whilecount = 1;
      int startstatement = conditionStatementsIndex;
      while (whilecount > 0 && loadCommand() && parsedollar() == STATE_OK) {
        if (strprefix(mcmd, "while ")) {
          whilecount++;
        }
        if (strcmp(mcmd, "done") == 0) {
          whilecount--;
          if (whilecount == 0) {
            break;
          }
        }
        if (conditionStatementsIndex >= 511) {
          strcpy(mshErrorInfo, "Too many statemnts saved for while loop!");
          return STATE_OVERFLOW;
        }
        strcpy(conditionStatements[conditionStatementsIndex++], ans);
      }
      int stopstatement = conditionStatementsIndex;
      conditionlist[conditionlistindex++] = parsewhile ? (WHILE_SIGN | CONDITION_ACCEPTED | (startstatement << 6) | (stopstatement << 19)) : (WHILE_SIGN | CONDITION_REJECTED | (startstatement << 6) | (stopstatement << 19));
      // Run commands!
      // Set a variable for function loadCommand() to tell it what to do!
      // Use loadvar to get the next loadCommand statement index;
      whilenextreadline[whilenextreadlineindex++] = startstatement;
      while (1) {
        whilenextreadline[whilenextreadlineindex - 1] = startstatement; 
        strcpy(tempwhilecondition, whilecondition);
        if (parseCondition(tempwhilecondition) == 0) {
          break;
        }
        while (whilenextreadline[whilenextreadlineindex - 1] < stopstatement) {
          if (!loadCommand()) {
            break;
          }
          if (parsedollar() != STATE_OK) {
            break;
          }
          // Check while
          if (strprefix(mcmd, "while ")) {
            // find where this while ends;
            int innerwhilecount = 1;
            int j;
            for (j = whilenextreadline[whilenextreadlineindex - 1]; j < stopstatement; j++) {
              if (strprefix(conditionStatements[j], "while ")) {
                innerwhilecount++;
              }
              else if (strcmp(conditionStatements[j], "done") == 0) {
                innerwhilecount--;
                if (innerwhilecount == 0) {
                  break;
                }
              }
            }
            if (j >= stopstatement) {
              strcpy(mshErrorInfo, "Parsing error! Can't find corresponding done!");
              return STATE_ERROR;
            }
            whilenextreadline[whilenextreadlineindex - 1] = j + 1;
          }
          mruncmd();
        }
      }
      // Already run all the commands!
      conditionStatementsIndex = stopstatement;
      conditionlistindex--;
      whilenextreadlineindex--;
      return STATE_OK;
    }
    else if (strcmp(mcmd, "do") == 0) {
      return STATE_OK;
    }
    else if (strcmp(mcmd, "done") == 0) {
      return STATE_OK;
    }
    if (conditionlistindex > 0 && (conditionlist[conditionlistindex - 1] & ACCEPTED_CHOSER) == 0) {
      return STATE_OK;
    }
    // check equal
    for (int j = 0; mcmd[j]; j++) {
      if (strchr(" \t\v\r\n", mcmd[j])) {
        break;
      }
      if (mcmd[j] == '=') {
        run = 0;
        mcmd[j] = 0;
        setVariables(mcmd, mcmd + j + 1);
      }
    }
  }
  if (run) {
    if(fork1() == 0)
      runcmd(parsecmd(mcmd));
    wait();
  }

  return STATE_OK;
}

int main(int argc, char*argv[])
{
  if (argc != 2) {
    printf(1, "Usage: msh COMMAND_FILE\n");
    exit();
  }

  if ((fd = open(argv[1], 0)) < 0) {
    printf(1, "Read file %s error!\n", argv[1]);
    exit();
  }
  
  strcpy(mshErrorInfo, "Sorry, no known ERROR info detected!");

  j = i = n = 0;
  while (loadCommand())
  {
    if (parsedollar() != STATE_OK) {
      printf(1, "Parse $ error!\n");
      continue;
    }
    if (mruncmd() != STATE_OK) {
      printf(1, "Error in msh running! Detailed info:\n  %s\n", mshErrorInfo);
      exit();
    }
  }

  exit();
}
