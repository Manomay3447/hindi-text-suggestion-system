#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <wctype.h>
#include"multigrams_hi.c"
#define MAX_WORDS 10000
#define MAX_WORDLEN 100

int isHindi(wchar_t ch) {
    return (ch >= 0x0900 && ch <= 0x097F);
}

void write_ngrams(FILE *foutptr, wchar_t words[][MAX_WORDLEN], int total_words) {
    for (int n = 2; n <= 5; n++) {
        for (int i = 0; i <= total_words - n; i++) {
            for (int j = 0; j < n; j++) {
                fputws(words[i + j], foutptr);
                if (j != n - 1) fputwc(L' ', foutptr);
            }
            fputwc(L'\n', foutptr);
        }
    }
}

void generateNgrams(int filecount, char *filepath[]) {
    setlocale(LC_ALL, "en_US.UTF-8");
    FILE *finptr, *foutptr;
    foutptr = fopen("ngrams.txt", "a");
    if (foutptr == NULL) {
        wprintf(L"Cannot open output file\n");
        exit(1);
    }

    for (int i = 0; i < filecount; i++) {
        finptr = fopen(filepath[i], "r");
        if (finptr == NULL) {
            wprintf(L"Cannot open input file: %s\n", filepath[i]);
            continue;
        }

        wchar_t words[MAX_WORDS][MAX_WORDLEN];
        int word_index = 0, char_index = 0;
        wint_t ch;

        while ((ch = fgetwc(finptr)) != WEOF) {
            if (iswspace(ch) || ch == L'।' || ch == L'.' || ch == L',' || ch == L'?' || ch == L'\'') {
                if (char_index > 0) {
                    words[word_index][char_index] = L'\0';
                    word_index++;
                    char_index = 0;
                    if (word_index >= MAX_WORDS) break;
                }
            } else if (isHindi(ch)) {
                if (char_index < MAX_WORDLEN - 1) {
                    words[word_index][char_index++] = ch;
                }
            }
        }

        // Add last word if needed
        if (char_index > 0 && word_index < MAX_WORDS) {
            words[word_index][char_index] = L'\0';
            word_index++;
        }

        write_ngrams(foutptr, words, word_index);

        fclose(finptr);
        wprintf(L"Generated n-grams from: %s\n", filepath[i]);
    }

    fclose(foutptr);
     grams();
}

