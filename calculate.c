#include "calculate.h"
#include "user.h"

// Consider that strlen(a) is less than 1024;
double nums[512];
int nump = 0;
char op[512];
int opp = 1;
int lastisop = 1;
int neg = 0;
int readpoint = 0;
double gotans = 0;
double base = 1;

int nowcal() {
	if (nump < 2) {
		return 0;
	}
	switch (op[--opp]) {
	case '+':
		nums[nump - 2] = nums[nump - 2] + nums[nump - 1];
		nump--;
		break;
	case '-':
		nums[nump - 2] = nums[nump - 2] - nums[nump - 1];
		nump--;
		break;
	case '/':
		nums[nump - 2] = nums[nump - 2] / nums[nump - 1];
		nump--;
		break;
	case '*':
		nums[nump - 2] = nums[nump - 2] * nums[nump - 1];
		nump--;
		break;
	default:
		return 0;
	}
	return 1;
}

int docal(char newa) {
	// left is 0, don't do anything
	if (op[opp - 1] == 0 || newa == '(' || op[opp - 1] == '(') {
		return 0;
	}
	// right is 0, do cal
	if (newa == 0 || newa == ')' || newa == '+' || newa == '-' || op[opp - 1] == '*' || op[opp - 1] == '/') {
		return 1;
	}
	return 0;
}

double calculate(char* a, int* ok) {
	if (strlen(a) > 1024) {
		*ok = -1;
		return .0;
	}
	op[0] = 0;
	while (1) {
		switch (*a) {
		case ' ':
			break;
		case '=':
			*a = 0;
		case '(':
		case ')':
		case '*':
		case '/':
		case '+':
		case '-':
		case 0:
			if ((*a == '+' || *a == '-') && (lastisop && op[opp - 1] != ')')) {
				neg = (*a == '-');
				break;
			}
			// Got num!
			if (!lastisop) {
				if (neg) {
					nums[nump++] = -gotans;
				}
				else {
					nums[nump++] = gotans;
				}
				gotans = 0;
				readpoint = 0;
				base = 1;
				neg = 0;
				lastisop = 1;
			}
			while (docal(*a)) {
				// Do cal
				if (!nowcal()) {
					*ok = -4;
					return .0;
				}
			}
			if (*a == 0) {
				goto OUTSIDE;
			}
			if (*a == ')') {
				if (op[opp - 1] == '(') {
					opp--;
				}
				else {
					*ok = -6;
					return .0;
				}
			}
			else {
				op[opp++] = *a;
			}
			break;
		default:
			lastisop = 0;
			if (*a == '.') {
				if (readpoint) {
					*ok = -2;
					return .0;
				}
				readpoint = 1;
				base /= 10;
			}
			else if (*a > '9' || *a < '0') {
				*ok = -3;
				return .0;
			}
			else if (readpoint) {
				gotans += base * (*a - '0');
				base /= 10;
			}
			else {
				gotans = gotans * 10 + *a - '0';
			}
			break;
		}
		if (*a == 0) {
			break;
		}
		a++;
	}
OUTSIDE:
	if (opp == 1 && nump == 1) {
		*ok = 0;
		return nums[0];
	}
	*ok = -5;
	return .0;
}