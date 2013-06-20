#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define LABELLENGTH 20
#define LABELNUM    50
#define INSTRUCTIONLENGTH 4000
#define LINELENGTH 200
#define OFFSET 2000
struct {
	char name[LABELLENGTH];
	unsigned long location;
}label[LABELNUM];
FILE *finput, *foutput1, *foutput2;

void getASCII(char byte[], char *c)
{
	int num = *c;
	int i;

	for (i = 0; i < 8; i++)
	{
		byte[7-i] = num % 2 + '0';
		num /= 2;
	}
}

int Match(char in[], char out[])
{
	int i, res, mark = 0;
	char reg[31][4] = {
		"$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0",
		"$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$s0",
		"$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8",
		"$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
	};
	char t[5];

	memset(t, '0', 5);
	if (!strncmp(in, "$zero", 5))
		for (i = 0; i < 5; ++i)
			out[i] = '0';
	else
	{
		for (i = 0; i < 31; ++i)
			if (!strncmp(reg[i], in, 3))
			{
				res = i+1;
				mark = 1;
				break;
			}
		if (mark == 0)
			return 1;
		for (i = 0; i < 5; ++i)
		{
			t[4-i] = res % 2 + '0';
			res >>= 1;
		}
		for (i = 0; i < 5; ++i)
			out[i] = t[i];
	}
	return 0;
}

int MatchLabel(char *p)
{
	int i, res = -1;

	for (i = 0; i < LABELNUM; i++)
		if (!strcmp(label[i].name, p))
		{
			res = i;
			break;
		}

	return res;
}

void R_type1(char s[], char in[], const char *c, int address)
{
	char *p = in;
	char *r[3];
	int i, err = 0;

	for (i = 0; *p != 0 && *p != '#' && *p != '\n'; p++)
	{
		if (*p == '$')
			r[i++] = p;
		if ((i == 3 && in[0] != 'd') || (i == 2 && in[0] == 'd'))
			break;
	}
	if ((i != 3 && in[0] != 'd') || (i != 2 && in[0] == 'd'))
	{
		printf("%d: Error Input!\n", address);
		system("pause");
		exit(1);
	}
	strncpy(s, "000000", 6);

	if (in[0] != 'd')
	{
		p = &s[6];
		err += Match(r[1], p);
		p = &s[11];
		err += Match(r[2], p);
		p = &s[16];
		err += Match(r[0], p);
	}
	else
	{
		p = &s[6];
		err += Match(r[0], p);
		p = &s[11];
		err += Match(r[1], p);
		p = &s[16];
		strncpy(p, "00000", 5);
	}

	p = &s[21];
	strncpy(p, "00000", 5);
	p += 5;
	strncpy(p, c, 6);
	if (err)
	{
		printf("%d: Error Register!\n", address);
		system("pause");
		exit(1);
	}
}

void R_type2(char s[], char in[], const char *c, int address)
{
	char *p = in;
	char *r[2];
	char t[5];
	int i, j = 0;
	int num = 0;
	int product = 1;
	int err = 0;
	memset(t, 0, 5);
	for (i = 0; *p != 0 && *p != '#' && *p != '\n'; p++)
	{
		if (*p == '$')
			r[i++] = p;
		if (*p == ',')
			j++;
		if (j == 2 && *p != ',')
			break;
	}
	if (j != 2)
	{
		printf("%d: Error Input!\n", address);
		system("pause");
		exit(1);
	}
	while(!(*p >= '0' && *p <= '9')) p++;
	while(*p >= '0' && *p <= '9') p++;
	p--;
	for ( ; *p != ' ' && *p != '\t' && *p != ','; p--)
	{
		num += (*p - '0') * product;
		product *= 10;
	}
	for (i = 0; i < 5; i++)
	{
		t[4-i] = num % 2 + '0';
		num /= 2;
	}
	strncpy(s, "00000000000", 11);
	p = &s[11];
	err += Match(r[1], p);
	p = &s[16];
	err += Match(r[0], p);
	p = &s[21];
	strncpy(p, t, 5);
	p += 5;
	strncpy(p, c, 6);
	if (err)
	{
		printf("%d: Error Register!\n", address);
		system("pause");
		exit(1);
	}
}

void R_type3(char s[], char in[], const char *c, int address)
{
	char *p = in;
	char *r;
	int mark = 0;
	int err = 0;
	for ( ; *p != 0 && *p != '#' && *p != '\n'; p++)
		if (*p == '$')
		{
			r = p;
			mark = 1;
			break;
		}
	if (mark == 0)
	{
		printf("%d: Error Input!\n", address);
		system("pause");
		exit(1);
	}
	strncpy(s, "000000", 6);
	p = &s[6];
	if (in[0] == 'j')
	{
		err += Match(r, p);
		p = &s[11];
		strncpy(p, "0000000000", 10);
	}
	else
	{
		strncpy(p, "0000000000", 10);
		p = &s[16];
		err += Match(r, p);
	}
	p = &s[21];
	strncpy(p, "00000", 5);
	p = &s[26];
	strncpy(p, c, 6);
	if (err)
	{
		printf("%d: Error Register!\n", address);
		system("pause");
		exit(1);
	}
}

void J_type(char s[], char in[], const char *c, int address2)
{
	char *p = in;
	char address[26];
	char num[10];
	int address_length = 0;
	int i;
	int number = 0;
	char l[LABELLENGTH];
	unsigned long sum = 0, product = 1;

	memset(address, 0, 26);
	memset(num, 0, 10);
	memset(l, 0, LABELLENGTH);

	for ( ; *p <= 'z' && *p >= 'a'; p++);
	for ( ; *p == ' ' || *p == '\t'; p++);

	//label
	if ((*p <= 'Z' && *p >= 'A') || (*p <= 'z' && *p >= 'a'))
	{
		for (i = 0; i < LABELLENGTH; ++i)
		{
			l[i] = *p;
			p++;
			if (*p == '\n' || *p == ' ' || *p == '\t' || *p == '\0' || *p == '#')
				break;
		}
		number = MatchLabel(l);
		if (number == -1)
		{
			printf("%d: No Label Matched!\n", address2);
			system("pause");
			exit(1);
		}
		sum = label[number].location;
	}
	else if (*p <= '9' && *p >= '0')
	{
		strcpy(num, p);
		for ( ; (num[address_length] != '\n') && (num[address_length] != 0) && (num[address_length] != '\t') && (num[address_length] != ' ' && (num[address_length] != '#')); address_length++);
		address_length--;
		if (*p >= '1')
		{
			for ( ; address_length >= 0; address_length--)
			{
				sum += (num[address_length]-'0') * product;
				product *= 10;
			}
		}
		else if (*p == '0' && *(p+1) == 'x')
		{
			for ( ; address_length >= 0; address_length--)
			{
				if (num[address_length] >= '0' && num[address_length] <= '9')
					sum += (num[address_length]-'0') * product;
				else if (num[address_length] >= 'a' && num[address_length] <= 'f')
					sum += (num[address_length]-'a'+10) * product;
				else
					sum += (num[address_length]-'A'+10) * product;
				product *= 16;
			}
		}
		else
		{
			printf("%d: Error Input!\n", address2);
			system("pause");
			exit(1);
		}
	}
	else
	{
		printf("%d: Error Input!\n", address2);
		system("pause");
		exit(1);
	}
	sum &= 0x3ffff;
	for (i = 0; i < 26; i++)
	{
		address[25-i] = sum % 2 + '0';
		sum /= 2;
	}
	strncpy(s, c, 6);
	strncpy(s+6, address, 26);
}

void I_type1(char s[], char in[], const char *c, int address)
{
	char *p = in;
	char *r[2];
	int i, j = 0, k = 0;
	int minusflag = 0;
	int labelmark = 0;
	int immediate_length = 0;
	int err = 0;
	char num[6];
	char immediate[16];
	char labelname[LABELLENGTH];
	unsigned long sum = 0, product = 1;
	memset(num, 0, 6);
	memset(immediate, 0, 16);
	for (i = 0; *p != 0 && *p != '#' && *p != '\n'; p++)
	{
		if (*p == '$')
			r[i++] = p;
		if (*p == ',')
			j++;
		if (*p == '-')
			minusflag = 1;
		if ((i == 2 && j == 2 && *p >= '0' && *p <= '9' && *(p+1) != 'x') || (i == 1 && j == 1 && *p >= '0' && *p <= '9' && *(p+1) != 'x' && in[0] == 'l'))
		{
			strncpy(num, p, 5);
			for ( ; (num[immediate_length] != '\n') && (num[immediate_length] != 0) && (num[immediate_length] != ' ') && (num[immediate_length] != '\t') && (num[immediate_length] != '#'); immediate_length++)
				;
			immediate_length--;
			for ( ; immediate_length >= 0; immediate_length--)
			{
				sum += (num[immediate_length]-'0') * product;
				product *= 10;
			}
			break;
		}
		else if ((i == 2 && j == 2 && *p == '0' && *(p+1) == 'x') || (i == 1 && j == 1 && *p == '0' && *(p+1) == 'x' && in[0] == 'l'))
		{
			for ( ; *(p+1) != '\n' && *(p+1) != '\0' && *(p+1) != '#' && *(p+1) != ' '; p++)
				;

			for ( ; *p != 'x'; p--)
			{
				if (*p >= 'a' && *p <= 'f')
					sum += (*p - 'a' + 10) * product;
				else if (*p >= 'A' && *p <= 'F')
					sum += (*p - 'A' + 10) * product;
				else
					sum += (*p - '0') * product;
				product *= 16;
			}
			break;
		}
		else if (((i == 2 && j == 2) || (i == 1 && j == 1)) && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')))
		{
			for (k = 0; *p != 0 && *p != '#' && *p != '\n' && *p != ' ' && *p != '\t'; k++)
				labelname[k] = *p++;
			labelname[k] = 0;
			labelmark = 1;
			break;
		}
	}
	if ((i != 2 && in[0] != 'l') || (i != 1 && in[0] == 'l'))
	{
		printf("%d: Error Input!\n", address);
		system("pause");
		exit(1);
	}

	if (minusflag)
		sum = (1<<16) - sum;
	if (!labelmark)
		for (i = 0; i < 16; i++)
		{
			immediate[15-i] = sum % 2 + '0';
			sum /= 2;
		}
	strncpy(s, c, 6);
	p = &s[6];
	if (in[0] != 'l')
		err += Match(r[1], p);
	else
		strncpy(p, "00000", 5);

	//Deal with la expansion
	if (!strncmp(in, "lui", 3) || !strncmp(in, "ori", 3))
	{
		if (labelmark)
		{
			i = MatchLabel(labelname);
			if (i == -1)
			{
				printf("%d: No Label Matched!\n", address);
				system("pause");
				exit(1);
			}
			if (!strncmp(in, "lui", 3))
				sum = label[i].location >> 16;
			else
				sum = label[i].location&0xffff;
			for (i = 0; i < 16; i++)
			{
				immediate[15-i] = sum % 2 + '0';
				sum /= 2;
			}
		}
	}

	p = &s[11];
	err += Match(r[0], p);
	p = &s[16];
	strncpy(p, immediate, 16);
	if (err)
	{
		printf("%d: Error Register!\n", address);
		system("pause");
		exit(1);
	}
}

void I_type2(char s[], char in[], const char *c, int address)
{
	int i, j;
	int err = 0;
	int hexmark = 0;
	char *p = in;
	char *r[2];
	char labelname[LABELLENGTH];
	char offset[16];
	unsigned long sum = 0, product = 1;
	memset(offset, 0, 16);

	for (i = j = 0; *p != 0 && *p != '#' && *p != '\n'; p++)
	{
		if (*p == '0' && *(p+1) == 'x')
			hexmark = 1;
		if (*p == '$')
			r[i++] = p;
		if (*p == ',')
			j++;
		if ((i == 2 && j == 1) || (i == 1 && j == 1 && 
					  	((*p <= 'Z' && *p >= 'A') || (*p <= 'z' && *p >= 'a')) &&
					  	((*(p+1) <= 'Z' && *(p+1) >= 'A') || (*(p+1) <= 'z' && *(p+1) >= 'a'))
					  )
		    )
			break;
	}
	if (i != 2 && !((*p <= 'Z' && *p >= 'A') || (*p <= 'z' && *p >= 'a')))
	{
		printf("%d: Error Input!\n", address);
		system("pause");
		exit(1);
	}

	if (i == 2)
	{
		p -= 2;
		while(*p == ' ' || *p == '\t')
			p--;
		if (hexmark)
			for ( ; *p != 'x'; p--)
			{
				if (*p >= 'a' && *p <= 'f')
					sum += (*p - 'a' + 10) * product;
				else if (*p >= 'A' && *p <= 'F')
					sum += (*p - 'A' + 10) * product;
				else
					sum += (*p - '0') * product;
				product *= 16;
			}
		else
			for ( ; *p != ' ' && *p != '-' && *p != ',' && *p != '\t'; p--)
			{
				sum += (*p - '0') * product;
				product *= 10;
			}

		if (*p == '-')
			sum = (1<<16) - sum;

		p = &s[6];
		err += Match(r[1], p);
		p = &s[11];
		err += Match(r[0], p);
	}
	else
	{
		memset(labelname, 0, LABELLENGTH);
		for (i = 0; i < LABELLENGTH; i++)
		{
			labelname[i] = *p;
			p++;
			if (*p == '\n' || *p == ' ' || *p == '\t' || *p == '\0' || *p == '#')
				break;
		}
		i = MatchLabel(labelname);
		if (i == -1)
		{
			printf("%d: No Label Matched!\n", address);
			system("pause");
			exit(1);
		}
		sum = label[i].location;
		p = &s[6];
		err += Match(r[0], p);
		p = &s[11];
		strncpy(p, "00000", 5);
	}

	for (i = 0; i < 16; i++)
	{
		offset[15-i] = sum % 2 + '0';
		sum /= 2;
	}
	strncpy(s, c, 6);
	p = &s[16];
	strncpy(p, offset, 16);
	if (err)
	{
		printf("%d: Error Register!\n", address);
		system("pause");
		exit(1);
	}
}

void I_type3(char s[], char in[], const char *c, int address)
{
	char *p = in;
	char *r[2];
	int i, j = 0;
	int err = 0;
	int hexmark = 0;
	char l[LABELLENGTH];
	char offset[16];
	unsigned long sum = 0, product = 1;
	memset(offset, 0, 16);
	memset(l, 0, LABELLENGTH);
	for (i = 0; *p != 0 && *p != '#' && *p != '\n'; p++)
	{
		if (*p == '$')
			r[i++] = p;
		if (*p == ',')
			j++;
		if (i == 2 && j == 2 && ( (*p <= '9' && *p >= '0') ||
								  ( (
								  		(*p <= 'Z' && *p >= 'A') || (*p <= 'z' && *p >= 'a')
								  	)
								  	&&
								    (
								    	(*(p+1) <= 'Z' && *(p+1) >= 'A') || (*(p+1) <= 'z' && *(p+1) >= 'a') || *(p+1) == '_'
								    )
								  )
								)
			)
			break;
	}

	if (*p == 0 || *p == '#' || *p == '\n')
	{
		printf("%d: Error Input!\n", address);
		system("pause");
		exit(1);
	}
	if (*p <= '9' && *p >= '0')
	{
		if (*p == '0' && *(p+1) == 'x')
			 hexmark = 1;
		while (*p != '\n' && *p != 0 && *p != ' ' && *p != '\t' && *p != '#')
			p++;
		p--;
		if (hexmark)
		{
			for ( ; *p != ' ' && *p != '-' && *p != '\t' && *p != ',' && *p != 'x'; p--)
			{
				if (*p >= 'a' && *p <= 'f')
					sum += (*p - 'a' + 10) * product;
				else if (*p >= 'A' && *p <= 'F')
					sum += (*p - 'A' + 10) * product;
				else
					sum += (*p - '0') * product;
				product *= 16;
			}
			if (*p == '-')
			{
				printf("%d: Error Hexadecimal Number!\n", address);
				system("pause");
				exit(1);
			}
		}
		else
		{
			for ( ; *p != ' ' && *p != '-' && *p != '\t' && *p != ','; p--)
			{
				sum += (*p - '0') * product;
				product *= 10;
			}
			if (*p == '-')
				sum = (1<<16) - sum;
		}
	}
	//label
	else
	{
		j = 0;
		while (*p != '\n' && *p != 0 && *p != ' ' && *p != '\t' && *p != '#')
		{
			l[j++] = *p;
			p++;
		}
		i = MatchLabel(l);
		if (i == -1)
		{
			printf("%d: No Label Matched!\n", address);
			system("pause");
			exit(1);
		}
		sum = label[i].location - (address+1);
		if (sum < 0)
			sum += (1<<16);
	}

	for (i = 0; i < 16; i++)
	{
		offset[15-i] = sum % 2 + '0';
		sum /= 2;
	}

	strncpy(s, c, 6);
	p = &s[6];
	err += Match(r[0], p);
	p = &s[11];
	err += Match(r[1], p);
	p = &s[16];
	strncpy(p, offset, 16);
	if (err)
	{
		printf("%d: Error Register!\n", address);
		system("pause");
		exit(1);
	}
}

void Syscall(char s[], char in[], const char *c, int address2)
{
	strncpy(s, "00000000000000000000000000001100", 32);
}

void C_type(char s[], char in[], const char *c, int address)
{
	int err = 0;
	char t[6];
	char *p = in;
	char *r;

	for (r = in; *r != '#' && *r != '\n' &&  *r != '\0'; r++)
		if (*r == '$')
			break;

	for (p = in; *p != '#' && *p != '\n' &&  *p != '\0'; p++)
		if (*p == 'S' || *p == 'C' || *p == 'E')
			break;
	if (*p == '#' || *p == '\n' ||  *p == '\0')
	{
		printf("%d: Error Coprocessor 0's Register!\n", address);
		system("pause");
		exit(1);
	}

	switch (*p)
	{
		case 'S':	strcpy(t, "01100");
					break;
		case 'C':	strcpy(t, "01101");
					break;
		case 'E':	strcpy(t, "01110");
					break;
	}

	//mfc0
	if (!strcmp("00000", c))
	{
		p = &s[11];
		strncpy(p, t, 5);
		p = &s[16];
	}
	//mtc0
	else
	{
		p = &s[16];
		strncpy(p, t, 5);
		p = &s[11];
	}
	strncpy(s, "010000", 6);
	strncpy(s+6, c, 5);
	err += Match(r, p);
	p = &s[21];
	strcpy(p, "00000000000");
	if (err)
	{
		printf("%d: Error Register!\n", address);
		system("pause");
		exit(1);
	}
}

void Psuedo_La(char s[][LINELENGTH], char in[], int index, char labelname[])
{
	int i;
	char *p = in;
	char lui[LINELENGTH] = "lui ";
	char ori[LINELENGTH] = "ori ";

	if (labelname[0])
	{
		strcpy(lui, labelname);
		strcat(lui, ": lui ");
	}

	for (p = in; ; p++)
		if (*p == '$')
			break;
	strncat(lui, p, 3);
	strcat(lui, ", ");
	strncat(ori, p, 3);
	strcat(ori, ", ");
	strncat(ori, p, 3);
	strcat(ori, ", ");

	for ( ; ; p++)
		if (	(
					(*p <= 'Z' && *p >= 'A') || (*p <= 'z' && *p >= 'a')
				)
				&&
				(
					(*(p+1) <= 'Z' && *(p+1) >= 'A') || (*(p+1) <= 'z' && *(p+1) >= 'a')
				)
			)
			break;
	strcat(lui, p);
	strcat(ori, p);

	for (i = 0; i < 30; i++)
	{
		if (lui[i] == '\n')
			lui[i] = 0;
		if (ori[i] == '\n')
			ori[i] = 0;
	}
	strcpy(s[index], lui);
	strcpy(s[index+1], ori);
}

int main(void)
{
	int i = 0, j = 0, k = 0, s = 0, t = 0;
	int length = 0, ins_length = 0;
	int datamark = 0;
	int textmark = 0;
	int hexmark = 0;
	int offset = OFFSET;
	int pc;
	char *p, *p2;
	char filename[100];
	char byte[8];
	char data[33];
	char labelname[LABELLENGTH];
	char in[INSTRUCTIONLENGTH][LINELENGTH];
	char ins[INSTRUCTIONLENGTH][LINELENGTH];
	char out[INSTRUCTIONLENGTH][33];
	char instruction[10];
	unsigned long mips = 0, num = 0, product = 1;

	//Initial
	for (i = 0; i < INSTRUCTIONLENGTH; i++)
	{
		memset(in[i], 0, LINELENGTH);
		memset(ins[i], 0, LINELENGTH);
		memset(out[i], 0, 33);
	}
	memset(filename, 0, 100);

	//Deal with files
	printf("Please enter the name of the file to be assembled.\nFilename: ");
	for ( ; ; )
	{
		memset(filename, 0, 100);
		gets(filename);
		strcat(filename, ".mips");
		finput = fopen(filename, "r");
		if (finput == NULL)
		{
			printf("Invalid Pointer! Please try again.\nFilename: ");
			continue;
		}
		filename[strlen(filename) - 5] = 0;
		strcat(filename, ".o");
		foutput1 = fopen(filename, "wb");
		filename[strlen(filename) - 2] = 0;
		strcat(filename, ".txt");
		foutput2 = fopen(filename, "w");
		if (foutput1 == NULL || foutput2 == NULL)
			printf("Invalid Pointer! Please try again.\nFilename: ");
		else
			break;
	}

	//Input
	for (i = 0; (fgets(in[i], LINELENGTH, finput) != NULL) && (i < INSTRUCTIONLENGTH); i++);  //load instruction into in[INSTRUCTIONLENGTH][40]
	length = i;

	//Deal with .data and .text
	pc = 0;
	for (i = 0; i < length; ++i)
	{
		labelname[0] = 0;
		for (j = 0; j < LINELENGTH; ++j)
		{
			if (in[i][j] == '.')
			{
				if (!strncmp(&in[i][j], ".data", 5))
				{
					datamark = 1;
					break;
				}
				if (!strncmp(&in[i][j], ".text", 5))
				{
					textmark = 1;
					break;
				}
			}

			if (datamark == 1 && textmark != 1)
			{
				//Deal with labels in data
				if (in[i][j] == ':')
				{
					p = &in[i][j] - 1;
					while (*p != 0 && *p != ' ' && *p != '\t' && *p != '\n')
						p--;
					p++;
					memset(labelname, 0, LABELLENGTH);
					for (s = 0; s < LABELLENGTH && p[s] != ':' && p[s] != ' ' && p[s] != '\t'; s++)
						labelname[s] = p[s];
					if (MatchLabel(labelname) != -1)
					{
						printf("%d: Error: Same Label!\n", i);
						system("pause");
						exit(1);
					}
					strcpy(label[k].name, labelname);
					label[k].location = pc;
					k++;
				}

				if (in[i][j] == '.')
				{
					if (!strncmp(&in[i][j], ".asciiz", 7))
					{
						p = &in[i][j] + 7;
						while(*p != '"')
							p++;
						p++;
						for ( ; *p != '"'; p += 4)
						{
							memset(byte, 0, 8);
							memset(data, 0, 33);
							for (s = 0; s < 4; s++)
							{
								if (*(p+s) == '"')
								{
									p += s;
									break;
								}
								getASCII(byte, p+s);
								strncat(data, byte, 8);
							}
							strcpy(out[t++], data);
							pc += 4;
							if (*p == '"')
							 	break;
						}

						break;
					}
					else if (!strncmp(&in[i][j], ".word", 5))
					{
						p2 = &in[i][j] + 5;
						for ( ; *p2 != '\n'; )
						{
							num = 0;
							hexmark = 0;
							product = 1;
							memset(data, 0, 33);

							while (*p2 == ' ' || *p2 == '\t' || *p2 == '-' || *p2 == ',')
								p2++;
							if (*p2 == '\n')
								break;
							for ( ; *p2 != ' ' && *p2 != '\t' && *p2 != ',' && *p2 != '\n'; p2++)
								if (*p2 == '0' && *(p2+1) == 'x')
									hexmark = 1;
							p = p2 - 1;

							if (hexmark)
								for ( ; *p != 'x'; p--)
								{
									if (*p >= 'a' && *p <= 'f')
										num += (*p - 'a' + 10) * product;
									else if (*p >= 'A' && *p <= 'F')
										num += (*p - 'A' + 10) * product;
									else
										num += (*p - '0') * product;
									product *= 16;
								}
							else
								for ( ; *p != ' ' && *p != '-' && *p != '\t'; p--)
								{
									num += (*p - '0') * product;
									product *= 10;
								}
							if (*p == '-')
								num = 0xffffffff - num + 1;

							for (s = 0; s < 32; s++)
							{
								data[31-s] = num % 2 + '0';
								num /= 2;
							}
							strcpy(out[t++], data);
							pc += 4;
						}
						break;
					}
					else if (!strncmp(&in[i][j], ".byte", 5))
					{
						num = 0;
						product = 1;
						memset(data, 0, 33);
						p2 = &in[i][j] + 5;

						hexmark = 0;
						while (*p2 == ' ' || *p2 == '\t' || *p2 == '-' || *p2 == ',')
							p2++;
						if (*p2 == '\n')
							break;
						for ( ; *p2 != ' ' && *p2 != '\t' && *p2 != ',' && *p2 != '\n'; p2++)
							if (*p2 == '0' && *(p2+1) == 'x')
								hexmark = 1;
						p = p2 - 1;

						if (hexmark)
							for ( ; *p != 'x'; p--)
							{
								if (*p >= 'a' && *p <= 'f')
									num += (*p - 'a' + 10) * product;
								else if (*p >= 'A' && *p <= 'F')
									num += (*p - 'A' + 10) * product;
								else
									num += (*p - '0') * product;
								product *= 16;
							}
						else
							for ( ; *p != ' ' && *p != '-' && *p != '\t'; p--)
							{
								num += (*p - '0') * product;
								product *= 10;
							}

						if (*p == '-')
						{
							printf("%d: Error Byte Length!\n", i);
							system("pause");
							exit(1);
						}

						if (num%4 == 0)
							num /= 4;
						else
							num = num / 4 + 1;
						for (s = 0; s < num; s++)
						{
							strcpy(out[t++], data);
							pc += 4;
						}

						break;
					}
				}
			}
			else if (textmark == 1 || datamark == 0)
			{
				//Deal with pseudo
				for (j = 0; j < LINELENGTH && in[i][j] != '#' && in[i][j] != '\n'; ++j)
					if (in[i][j] == ':')
						break;

				if (in[i][j] == ':')
				{
					p = &in[i][j] - 1;
					while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
						p--;
					p++;
					for (s = 0; s < LABELLENGTH && p[s] != ':' && p[s] != ' ' && p[s] != '\t'; s++)
						labelname[s] = p[s];
					labelname[s] = 0;
				}

				if (j == LINELENGTH || in[i][j] == '#' || in[i][j] == '\n')
					j = 0;

				for ( ; j < LINELENGTH && in[i][j] != '#' && in[i][j] != '\n'; ++j)
					if (in[i][j] <= 'z' && in[i][j] >= 'a')
						break;

				if (in[i][j] <= 'z' && in[i][j] >= 'a')
				{
					p = &in[i][j];
					for (s = 0; s < 10 && p[s] != ' ' && p[s] != '\n' && p[s] != '\t' && p[s] != '\0' && p[s] != ',' && p[s] != ':'; s++)
						instruction[s] = p[s];
					if (!strncmp(p, "syscall", 7) && p[s] == ':')
						instruction[s++] = ':';
					instruction[s] = 0;

					if (!strcmp(instruction, "la"))
					{
						Psuedo_La(ins, p, ins_length, labelname);
						ins_length += 2;
					}
					else
						strcpy(ins[ins_length++], in[i]);

					break;
				}
			}
		}
	}

	//Deal with labels in text
	for (i = 0; i < ins_length; ++i)
	{
		for (j = 0; j < LINELENGTH; ++j)
			if (ins[i][j] == ':')
				break;
		if (j == LINELENGTH)
			continue;
		else
			if (ins[i][j] == ':')
		{
			p = &ins[i][j] - 1;
			while (*p != 0 && *p != ' ' && *p != '\t' && *p != '\n')
				p--;
			p++;
			memset(labelname, 0, LABELLENGTH);
			for (s = 0; s < LABELLENGTH && p[s] != ':' && p[s] != ' ' && p[s] != '\t'; s++)
				labelname[s] = p[s];
			if (MatchLabel(labelname) != -1)
			{
				printf("%d: Error: Same Label!\n", i);
				system("pause");
				exit(1);
			}
			strcpy(label[k].name, labelname);
			label[k].location = i + offset;
			k++;
		}

		if (k == LABELNUM)
		{
			printf("Error: Exceed storge!\n");
			return 0;
		}
	}

	//Assemble
	for (i = 0; i < ins_length; ++i)
	{
		for (j = 0; j < LINELENGTH && ins[i][j] != '#' && ins[i][j] != '\n'; ++j)
			if (ins[i][j] == ':')
				break;
		if (j == LINELENGTH || ins[i][j] == '#' || ins[i][j] == '\n')
			j = 0;

		for ( ; j < LINELENGTH && ins[i][j] != '#' && ins[i][j] != '\n'; ++j)
			if (ins[i][j] <= 'z' && ins[i][j] >= 'a')
				break;

		p = &ins[i][j];
		for (s = 0; s < 10 && p[s] != ' ' && p[s] != '\n' && p[s] != '\t' && p[s] != '\0' && p[s] != ',' && p[s] != ':'; s++)
			instruction[s] = p[s];
		if (!strncmp(p, "syscall", 7) && p[s] == ':')
			instruction[s++] = ':';
		instruction[s] = 0;

		if (!strcmp(instruction, "add"))        R_type1(out[i+offset], p, "100000", i+offset);
		else if (!strcmp(instruction, "addu"))  R_type1(out[i+offset], p, "100001", i+offset);
		else if (!strcmp(instruction, "sub"))   R_type1(out[i+offset], p, "100010", i+offset);
		else if (!strcmp(instruction, "subu"))  R_type1(out[i+offset], p, "100011", i+offset);
		else if (!strcmp(instruction, "and"))   R_type1(out[i+offset], p, "100100", i+offset);
		else if (!strcmp(instruction, "or"))    R_type1(out[i+offset], p, "100101", i+offset);
		else if (!strcmp(instruction, "xor"))   R_type1(out[i+offset], p, "100110", i+offset);
		else if (!strcmp(instruction, "nor"))   R_type1(out[i+offset], p, "100111", i+offset);
		else if (!strcmp(instruction, "slt"))   R_type1(out[i+offset], p, "101010", i+offset);
		else if (!strcmp(instruction, "sltu"))  R_type1(out[i+offset], p, "101011", i+offset);
		else if (!strcmp(instruction, "sllv"))  R_type1(out[i+offset], p, "000100", i+offset);
		else if (!strcmp(instruction, "srlv"))  R_type1(out[i+offset], p, "000110", i+offset);
		else if (!strcmp(instruction, "srav"))  R_type1(out[i+offset], p, "000111", i+offset);
		else if (!strcmp(instruction, "div"))   R_type1(out[i+offset], p, "011010", i+offset);

		else if (!strcmp(instruction, "sll"))   R_type2(out[i+offset], p, "000000", i+offset);
		else if (!strcmp(instruction, "srl"))   R_type2(out[i+offset], p, "000010", i+offset);
		else if (!strcmp(instruction, "sra"))   R_type2(out[i+offset], p, "000011", i+offset);

		else if (!strcmp(instruction, "jr"))    R_type3(out[i+offset], p, "001000", i+offset);
		else if (!strcmp(instruction, "mfhi"))  R_type3(out[i+offset], p, "010000", i+offset);
		else if (!strcmp(instruction, "mflo"))  R_type3(out[i+offset], p, "010010", i+offset);

		//j_type
		else if (!strcmp(instruction, "j"))     J_type(out[i+offset], p, "000010", i+offset);
		else if (!strcmp(instruction, "jal"))   J_type(out[i+offset], p, "000011", i+offset);

		//i_type
		else if (!strcmp(instruction, "addi"))  I_type1(out[i+offset], p, "001000", i+offset);
		else if (!strcmp(instruction, "addiu")) I_type1(out[i+offset], p, "001001", i+offset);
		else if (!strcmp(instruction, "andi"))  I_type1(out[i+offset], p, "001100", i+offset);
		else if (!strcmp(instruction, "ori"))   I_type1(out[i+offset], p, "001101", i+offset);
		else if (!strcmp(instruction, "xori"))  I_type1(out[i+offset], p, "001110", i+offset);
		else if (!strcmp(instruction, "lui"))   I_type1(out[i+offset], p, "001111", i+offset);
		else if (!strcmp(instruction, "slti"))  I_type1(out[i+offset], p, "001010", i+offset);
		else if (!strcmp(instruction, "sltiu")) I_type1(out[i+offset], p, "001011", i+offset);

		else if (!strcmp(instruction, "lw"))    I_type2(out[i+offset], p, "100011", i+offset);
		else if (!strcmp(instruction, "sw"))    I_type2(out[i+offset], p, "101011", i+offset);
		else if (!strcmp(instruction, "lb"))    I_type2(out[i+offset], p, "100000", i+offset);
		else if (!strcmp(instruction, "sb"))    I_type2(out[i+offset], p, "101000", i+offset);
		else if (!strcmp(instruction, "lh"))    I_type2(out[i+offset], p, "100001", i+offset);
		else if (!strcmp(instruction, "sh"))    I_type2(out[i+offset], p, "101001", i+offset);

		else if (!strcmp(instruction, "beq"))   I_type3(out[i+offset], p, "000100", i+offset);
		else if (!strcmp(instruction, "bne"))   I_type3(out[i+offset], p, "000101", i+offset);

		//syscall
		else if (!strcmp(instruction, "syscall"))     Syscall(out[i+offset], p, "001100", i+offset);
		else if (!strcmp(instruction, "syscall:"))    {j += 7;continue;}

		//mfc0, mtc0
		else if (!strcmp(instruction, "mfc0"))  C_type(out[i+offset], p, "00000", i+offset);
		else if (!strcmp(instruction, "mtc0"))  C_type(out[i+offset], p, "00100", i+offset);

		//Deal with wrong input
		else
		{
			printf("%d: Wrong Input!\n", i);
			system("pause");
			exit(1);
		}

	}

	for (i = 0; i < INSTRUCTIONLENGTH/2 + ins_length; i++)
	{
		for (j = 0; j < 32; j++)
		{
			mips <<= 1;
			if (out[i][j] == '0' || out[i][j] == '1')
				mips += (unsigned long)out[i][j]-'0';
			else
				break;
		}
		fwrite(&mips, sizeof(mips), 1, foutput1);
		fputs(out[i], foutput2);
		fputc('\n',   foutput2);
	}

	printf("\nDone.\nEnjoy it~\n");
	fclose(finput);
	fclose(foutput1);
	fclose(foutput2);
	system("pause");
	return 0;
}
