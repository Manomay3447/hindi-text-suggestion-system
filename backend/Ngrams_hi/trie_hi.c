#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#define MAX_CHILDREN 128            
#define UNICODE_BASE 0x0900         // Start of Devanagari block
#define MAX_WORD_LEN 256
#define MAX_UNIGRAM_SUGGESTIONS 10
#define MAX_WORD_LENGTH 100

// Trie Node
typedef struct unigramTrieNode {
    struct unigramTrieNode *children[MAX_CHILDREN];
    int isEndOfWord;
    int frequency;
} unigramTrieNode;
   
typedef struct {
    wchar_t word[MAX_WORD_LENGTH];
    int frequency;
} UnigramSuggestion;

void collectUnigrams(unigramTrieNode *node, wchar_t *current, int depth, UnigramSuggestion *suggestions, int *count) {
    if (!node || *count >= MAX_UNIGRAM_SUGGESTIONS)
        return;

    if (node->isEndOfWord) {
        current[depth] = L'\0';
        wcscpy(suggestions[*count].word, current);
        suggestions[*count].frequency = node->frequency;
        (*count)++;
    }

    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (node->children[i]) {
            current[depth] = (wchar_t)i;
            collectUnigrams(node->children[i], current, depth + 1, suggestions, count);
        }
    }
}

int compareUnigramsByFrequency(const void *a, const void *b) {
    return ((UnigramSuggestion *)b)->frequency - ((UnigramSuggestion *)a)->frequency;
}

wchar_t **searchUnigramSuggestions(unigramTrieNode *root, int *resultCount) {
    static wchar_t *results[MAX_UNIGRAM_SUGGESTIONS];
    UnigramSuggestion suggestions[MAX_UNIGRAM_SUGGESTIONS];
    wchar_t current[MAX_WORD_LENGTH];
    *resultCount = 0;

    collectUnigrams(root, current, 0, suggestions, resultCount);

    qsort(suggestions, *resultCount, sizeof(UnigramSuggestion), compareUnigramsByFrequency);

    for (int i = 0; i < *resultCount; i++) {
        results[i] = (wchar_t *)malloc(sizeof(wchar_t) * (wcslen(suggestions[i].word) + 1));
        wcscpy(results[i], suggestions[i].word);
    }

    return results;
}

// Create Trie Node
unigramTrieNode* createUnigramNode() {
    unigramTrieNode *node = (unigramTrieNode*)malloc(sizeof(unigramTrieNode));
    if (!node) {
        perror("Failed to allocate TrieNode");
        exit(1);
    }
    node->isEndOfWord = 0;
    node->frequency = 0;
    for (int i = 0; i < MAX_CHILDREN; i++)
        node->children[i] = NULL;
    return node;
}

// Check if wchar_t is punctuation
int isPunctuation(wchar_t ch) {
    return (ch == L',' || ch == L'.' || ch == L':' || ch == L';' ||
            ch == L'!' || ch == L'?' || ch == L'\'' || ch == L'\"' ||
            ch == L'|' || ch == L'।');
}

// Clean punctuations from word
void cleanPunctuation(wchar_t *word) {
    wchar_t cleaned[MAX_WORD_LEN];
    int j = 0;

    for (int i = 0; word[i] != L'\0'; i++) {
        if (!isPunctuation(word[i])) {
            cleaned[j++] = word[i];
        }
    }

    cleaned[j] = L'\0';
    wcscpy(word, cleaned);
}

// Insert word into Trie (Devanagari optimized)
void insert(unigramTrieNode *root, const wchar_t *word) {
     
    if (word == NULL || wcslen(word) == 0)
    	return;
    unigramTrieNode *current = root;

    while (*word) {
        int offset = *word - UNICODE_BASE;

        if (offset < 0 || offset >= MAX_CHILDREN) {
            word++;  // Skip unsupported characters
            continue;
        }

        if (current->children[offset] == NULL)
            current->children[offset] = createUnigramNode();

        current = current->children[offset];
        word++;
    }

    current->isEndOfWord = 1;
    current->frequency += 1;
}

// Display Trie
void displayUnigramTrie(unigramTrieNode *root, wchar_t *buffer, int depth) {
    if (root->isEndOfWord) {
        buffer[depth] = L'\0';
        wprintf(L"Word: %ls | Frequency: %d\n", buffer, root->frequency);
    }

    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (root->children[i]) {
            buffer[depth] = (wchar_t)(i + UNICODE_BASE);
            displayUnigramTrie(root->children[i], buffer, depth + 1);
        }
    }
}

// Free Trie
void freeUnigramTrie(unigramTrieNode *root) {
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (root->children[i])
            freeUnigramTrie(root->children[i]);
    }
    free(root);
}

// Process line: tokenize, clean, insert
void processLine(unigramTrieNode *root, wchar_t *line) {
    const wchar_t *delimiters = L" \t\n\r";
    wchar_t *saveptr = NULL;
    wchar_t *token = wcstok(line, delimiters, &saveptr);

    while (token != NULL) {
        cleanPunctuation(token);

        // Check if token is non-empty and not just whitespace
        int valid = 0;
        for (size_t i = 0; token[i] != L'\0'; i++) {
            if (!iswspace(token[i])) {
                valid = 1;
                break;
            }
        }

        if (valid) {
            // Debug: print token being inserted
            //wprintf(L"Inserting: [%ls]\n", token);
            insert(root, token);
        } else {
            // Debug: print skipped token
            wprintf(L"Skipping empty/space-only token: [%ls]\n", token);
        }

        token = wcstok(NULL, delimiters, &saveptr);
    }
}


unigramTrieNode *buildUnigramTrie(int filecount, char *filepath[]) {
    
    setlocale(LC_ALL, "");
    unigramTrieNode *root = createUnigramNode();
    wchar_t line[1024];
    wchar_t buffer[MAX_WORD_LEN];

    for (int i = 0; i < filecount; i++) {
    FILE *file = fopen(filepath[i], "r, ccs=UTF-8");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    while (fgetws(line, sizeof(line) / sizeof(wchar_t), file)) {
        // Remove trailing newline
        wchar_t *pos;
        if ((pos = wcschr(line, L'\n')) != NULL) *pos = L'\0';
        if ((pos = wcschr(line, L'\r')) != NULL) *pos = L'\0';

        processLine(root, line);
    }

    fclose(file);
    }
    //wprintf(L"Words in Trie with Term Frequencies:\n");
    //displayUnigramTrie(root, buffer, 0);
    
    return(root);
}

