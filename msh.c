#include "shlib.h"
#include "stack.h"

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
// (conditionlist[i] << 13) >> 6 for start index of conditionStatements
// conditionlist[i] >> 19 for end index of conditionStatements(not included)
int conditionlist[128];
int conditionlistindex = 0;

// for for, while, until loop
char conditionStatements[512][512];

int strprefix(char *str ,char *pre)
{
  if (strlen(str) < strlen(pre)) {
    return 0;
  }
  while(*str && *pre && *str == *pre){
    str++;
    pre++;
  }
  if(!*pre)
    return 1;
  return 0;
}

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

char* strimAndTrip(char* a) {
  while (*a && strchr(" \t\v\r\n", *a)) {
    a++;
  }
  int i;
  for (i = strlen(a) - 1; i >= 0; i--) {
    if (a[i]) {
      break;
    }
  }
  a[i + 1] = 0;
  return a;
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
	index = subString(condition, "=");
	if (index >= 0) {
		condition[index] = 0;
		return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + 1 + (condition[index + 1] == '='))) == 0;
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
	index = subString(condition, ">");
	operationlen = 1;
	if (index >= 0) {
		condition[index] = 0;
		return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen)) > 0;
	}
	index = subString(condition, ">=");
	operationlen = 2;
	if (index >= 0) {
		condition[index] = 0;
		return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen)) >= 0;
	}
	index = subString(condition, "<");
	operationlen = 1;
	if (index >= 0) {
		condition[index] = 0;
		return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen)) < 0;
	}
	index = subString(condition, "<=");
	operationlen = 2;
	if (index >= 0) {
		condition[index] = 0;
		return strcmpParsingDollar(strimAndTrip(condition), strimAndTrip(condition + index + operationlen)) <= 0;
	}
	return 0;
}

STATE setVariables(char* name, char* value) {
  for (int i = 0; i < vlen; i++) {
    if (strcmp(name, vs[i].name) == 0) {
      if (strlen(value) >= DEFALUT_VARIABLE_CONTENT) {
        return STATE_OVERFLOW;
      }
      strcpy(vs[i].value, value);
      return STATE_OK;
    }
  }
  if (vlen == vsize || strlen(name) >= DEFALUT_VARIABLE_NAME) {
    return STATE_OVERFLOW;
  }
  strcpy(vs[vlen].name, name);
  strcpy(vs[vlen].value, value);
  vlen++;
  return STATE_OK;
}

int loadCommand() {
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
  parsedollarindex = 0;
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
      if (quote == '\"') {
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
            return STATE_ERROR;
          }
        }
        if (getvariable() == STATE_ERROR) {
          return STATE_ERROR;
        }
        break;
      }
      default:
        mcmd[mcmdindex++] = ans[parsedollarindex];
        if (mcmdindex >= 2048) {
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
    if (strprefix(mcmd, "if ")) {
      if (conditionlistindex >= 128) {
        return STATE_OVERFLOW;
      }
      int parseif = parseCondition(mcmd + 3);
      conditionlist[conditionlistindex++] = parseif ? (IF_SIGN | CONDITION_ACCEPTED) : (IF_SIGN | CONDITION_REJECTED);
      return STATE_OK;
    }
    else if (strprefix(mcmd, "then")) {
      return STATE_OK;
    }
    else if (strprefix(mcmd, "elif ")) {
      if (conditionlistindex >= 128) {
        return STATE_OVERFLOW;
      }
      if (conditionlistindex < 1 || ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != IF_SIGN) && ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != ELIF_SIGN)) {
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
    else if (strprefix(mcmd, "else")) {
      if (conditionlistindex >= 128) {
        return STATE_OVERFLOW;
      }
      if (conditionlistindex < 1 || ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != IF_SIGN) && ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != ELIF_SIGN)) {
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
    else if (strprefix(mcmd, "fi")) {
      if (conditionlistindex < 1 || ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != IF_SIGN) && ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != ELIF_SIGN) && ((conditionlist[conditionlistindex - 1] & CONDITION_CHOSER) != ELSE_SIGN)) {
        return STATE_ERROR;
      }
      while (conditionlistindex > 0) {
        if ((conditionlist[--conditionlistindex] & CONDITION_CHOSER) == IF_SIGN) {
          break;
        }
      }
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
  
  j = i = n = 0;
  while (loadCommand())
  {
    if (parsedollar() != STATE_OK) {
      printf(1, "Parse $ error!\n");
      continue;
    }
    if (mruncmd() != STATE_OK) {
      printf(1, "Error in msh running!\n");
      exit();
    }
  }

  exit();
}
