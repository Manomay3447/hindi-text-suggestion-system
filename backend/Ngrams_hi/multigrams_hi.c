#include<stdio.h>
#include<stdlib.h>
#include<wchar.h>
#include<locale.h>

void sk(int cnt, wchar_t *temp, FILE *grams[], wint_t ch)
{
    setlocale(LC_ALL, "");

    // Valid cnt values: 1 to 4 (i.e., 2-gram to 5-gram)
    if (cnt >= 1 && cnt <= 4)
    {
        fputws(temp, grams[cnt - 1]);
        fputwc(ch, grams[cnt - 1]);
    }

    // Reset temp buffer safely
    temp[0] = L'\0';
}

void grams()
{
    setlocale(LC_ALL, "");

    FILE *grams[5];
    wchar_t *temp;
    int ch, cnt = 0, cap = 0, size = 32;

    // Allocate initial buffer
    temp = malloc(size * sizeof(wchar_t));

    // Open files
    grams[0] = fopen("2grms.txt", "w");
    grams[1] = fopen("3grms.txt", "w");
    grams[2] = fopen("4grms.txt", "w");
    grams[3] = fopen("5grms.txt", "w");
    grams[4] = fopen("ngrams.txt", "r");

    if (!temp || !grams[4]) {
        fwprintf(stderr, L"File open or memory error.\n");
        return;
    }

    while ((ch = fgetwc(grams[4])) != WEOF)
    {
        if (ch == L'\n')
        {
            temp[cap] = L'\0';  // Null-terminate before sending
            sk(cnt, temp, grams, ch);
            cnt = cap = 0;
            continue;
        }

        // Count spaces (to determine n-gram size)
        if (ch == L' ')
            cnt++;

        // Resize buffer if needed
        if (cap + 1 >= size)
        {
            size *= 2;
            temp = realloc(temp, size * sizeof(wchar_t));
            if (!temp) {
                fwprintf(stderr, L"Memory reallocation failed.\n");
                break;
            }
        }

        temp[cap++] = ch;
    }

    // Close all files
    for (int i = 0; i < 5; i++)
        fclose(grams[i]);

    free(temp);
}

