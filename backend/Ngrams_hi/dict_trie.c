#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

#define MAX_CHILDREN 128            
#define UNICODE_BASE 0x0900         
#define MAX_WORD_LEN 256
#define MAX_UNIGRAM_SUGGESTIONS 10
#define MAX_WORD_LENGTH 100

// Trie Node definition
typedef struct TrieNode {
    struct TrieNode *children[MAX_CHILDREN]; // Assuming UTF-8 index mapping
    int isWord;       // 1 if it's a complete dictionary word
    int frequency;    // frequency count for unigram
} TrieNode;

typedef struct {
    wchar_t word[MAX_WORD_LENGTH];
    int frequency;
} UnigramSuggestion;

// Function to create a new TrieNode
TrieNode *createTrieNode() {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (node == NULL) {
        fwprintf(stderr, L"Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CHILDREN; i++) {
        node->children[i] = NULL;
    }

    node->isWord = 0;
    node->frequency = 0;

    return node;
}

void collectUnigrams(TrieNode *node, wchar_t *current, int depth, UnigramSuggestion *suggestions, int *count) {
    if (!node || *count >= MAX_UNIGRAM_SUGGESTIONS)
        return;

    if (node->isWord) {
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

wchar_t **searchUnigramSuggestions(TrieNode *root, int *resultCount) {
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

int getOffset(wchar_t ch) {
    int offset = ch - UNICODE_BASE;
    return (offset >= 0 && offset < MAX_CHILDREN) ? offset : -1;
}

wchar_t getCharFromIndex(int index) {
    return (wchar_t)(UNICODE_BASE + index);  // Inverse of getOffset
}

int isPunctuation(wchar_t ch) {
    return (ch == L',' || ch == L'.' || ch == L':' || ch == L';' ||
            ch == L'!' || ch == L'?' || ch == L'\'' || ch == L'\"' ||
            ch == L'|' || ch == L'।');
}

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

// Insert a word into the Trie
void insertUnigram(TrieNode *root, const wchar_t *word) {
    TrieNode *curr = root;
    for (int i = 0; word[i] != L'\0'; i++) {
        int offset = getOffset(word[i]);
        if (offset == -1) continue;

        if (!curr->children[offset])
            curr->children[offset] = createTrieNode();

        curr = curr->children[offset];
    }
    curr->frequency++;
}

void processLine(TrieNode *root, wchar_t *line) {
    const wchar_t *delimiters = L" \t\n\r";
    wchar_t *saveptr = NULL;
    wchar_t *token = wcstok(line, delimiters, &saveptr);

    while (token != NULL) {
        cleanPunctuation(token);

        int valid = 0;
        for (size_t i = 0; token[i] != L'\0'; i++) {
            if (!iswspace(token[i])) {
                valid = 1;
                break;
            }
        }

        if (valid) {
            insertUnigram(root, token);  // Use frequency field
        } else {
            // Debug: Skipped token
            wprintf(L"Skipping invalid token: [%ls]\n", token);
        }

        token = wcstok(NULL, delimiters, &saveptr);
    }
}

int searchDict(TrieNode *root, const wchar_t *word) {
    TrieNode *node = root;
    int offset;

    while (*word) {
        offset = *word - UNICODE_BASE;
        if (offset < 0 || offset >= MAX_CHILDREN) {
            word++;  // Skip unsupported characters
            continue;
        }

        if (!node->children[offset]) return 0;
        node = node->children[offset];
        word++;
    }

    return node && (node->isWord || node->frequency > 0);
}

TrieNode* searchPrefix(TrieNode *root, const wchar_t *prefix) {
    TrieNode *node = root;
    int offset;

    while (*prefix) {
        offset = *prefix - UNICODE_BASE;
        if (offset < 0 || offset >= MAX_CHILDREN || !node->children[offset]) {
            return NULL; // No valid prefix path
        }
        node = node->children[offset];
        prefix++;
    }
    return node; // Return node where prefix ends
}


void insertDictWord(TrieNode *root, const wchar_t *word) {
    TrieNode *curr = root;
    for (int i = 0; word[i] != L'\0'; i++) {
        int offset = getOffset(word[i]);
        if (offset == -1) continue;

        if (!curr->children[offset])
            curr->children[offset] = createTrieNode();

        curr = curr->children[offset];
    }
    curr->isWord = 1;
}


// Recursive function to display all words in Trie
void displayTrie(TrieNode *root, wchar_t *buffer, int depth) {
    if (root->isWord || root->frequency > 0) {
        buffer[depth] = L'\0';
        wprintf(L"%ls", buffer);
        if (root->isWord) wprintf(L" [Dict]");
        if (root->frequency > 0) wprintf(L" [Freq: %d]", root->frequency);
        wprintf(L"\n");
    }

    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (root->children[i]) {
            buffer[depth] = getCharFromIndex(i);
            displayTrie(root->children[i], buffer, depth + 1);
        }
    }
}

// Free the Trie memory
void freeDictTrie(TrieNode *root) {
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (root->children[i])
            freeDictTrie(root->children[i]);
    }
    free(root);
}

TrieNode *buildUnifiedTrie(int unigramCount, char *unigramPaths[],int dictCount, char *dictPaths[]) {
    setlocale(LC_ALL, "");
    TrieNode *root = createTrieNode();
    wchar_t line[1024];

    // Process unigram files
    for (int i = 0; i < unigramCount; i++) {
        FILE *file = fopen(unigramPaths[i], "r, ccs=UTF-8");
        if (!file) {
            perror("Error opening unigram file");
            exit(1);
        }

        while (fgetws(line, sizeof(line) / sizeof(wchar_t), file)) {
            wchar_t *pos;
            if ((pos = wcschr(line, L'\n')) != NULL) *pos = L'\0';
            if ((pos = wcschr(line, L'\r')) != NULL) *pos = L'\0';

            processLine(root, line);
        }

        fclose(file);
    }

    // Process dictionary files
    for (int i = 0; i < dictCount; i++) {
        FILE *file = fopen(dictPaths[i], "r, ccs=UTF-8");
        if (!file) {
            perror("Error opening dictionary file");
            exit(1);
        }

        while (fgetws(line, sizeof(line) / sizeof(wchar_t), file)) {
            wchar_t *pos;
            if ((pos = wcschr(line, L'\n')) != NULL) *pos = L'\0';
            if ((pos = wcschr(line, L'\r')) != NULL) *pos = L'\0';

            cleanPunctuation(line);
            if (wcslen(line) > 0)
                insertDictWord(root, line);  // Only mark isWord=1
        }

        fclose(file);
    }

    return root;
}

