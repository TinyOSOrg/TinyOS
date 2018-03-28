int main(void)
{
    while(1)
        *(char*)0xc00b8000 = 'C';
    return 0;
}
