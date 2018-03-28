
void SetChar(int idx, char ch)
{
    *((char*)0xc00b8000 + (idx << 1)) = ch;
}

void PrintStr(const char *str)
{
    int pos = 0;
    while(*str)
        SetChar(pos++, *str++);
}
