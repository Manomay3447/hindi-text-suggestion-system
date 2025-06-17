#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#define UNICODE_BASE 0x0900      // Start of Devanagari block
#define MAX_DEVA_CHARS 128        
#define MAX_NGRAM_LEN 512
#define MAX_RESULTS 20
#define MAX_PHRASE_LEN 100

typedef struct ngramTrieNode {
    struct ngramTrieNode *children[MAX_DEVA_CHARS + 1];  //1 for space character
    int isEndOfWord;
    int frequency;
} ngramTrieNode;

typedef struct {
    wchar_t phrase[MAX_PHRASE_LEN];
    int frequency;
} Suggestion;

int compareSuggestions(const void *a, const void *b) {
    return ((Suggestion *)b)->frequency - ((Suggestion *)a)->frequency;
}

void collectSuggestions(ngramTrieNode *node, wchar_t *buffer, int depth,Suggestion *suggestions, int *count) {
    if (!node || *count >= MAX_RESULTS) return;

    if (node->isEndOfWord) {
        buffer[depth] = L'\0';
        wcscpy(suggestions[*count].phrase, buffer);
        suggestions[*count].frequency = node->frequency;
        (*count)++;
    }

    for (int i = 0; i <= MAX_DEVA_CHARS; i++) {
        if (node->children[i]) {
            if (i == 0){
    		buffer[depth] = L' ';
	    }
	    else{
	    	wchar_t ch = (wchar_t)(UNICODE_BASE + i - 1);
            	if (ch < 0x0900 || ch > 0x097F) continue; // Skip invalid Unicode
            	buffer[depth] = ch;
	    }
	    collectSuggestions(node->children[i], buffer, depth + 1, suggestions, count);
        }
    }
}

ngramTrieNode* traverseContext(ngramTrieNode *root, wchar_t *context) {
    ngramTrieNode *node = root;

    for (int i = 0; context[i] != L'\0'; i++) {
        int index;
        if (context[i] == L' ') {
            index = 0;
        } else {
            index = (context[i] - UNICODE_BASE) + 1;
            if (index <= 0 || index > MAX_DEVA_CHARS) {
                continue;  // skip unsupported character
            }
        }

        if (!node->children[index])
            return NULL;

        node = node->children[index];
    }

    return node;
}


wchar_t** searchNgramSuggestions(wchar_t *w1, wchar_t *w2, wchar_t *w3, wchar_t *w4,ngramTrieNode *root, int *count) {
    *count = 0;
    fwprintf(stderr, L"Context: w1=%ls w2=%ls w3=%ls w4=%ls\n", w1, w2, w3, w4);
    wchar_t context[256] = L"";
    if (w1) wcscat(context, w1);
    if (w2) { wcscat(context, L" "); wcscat(context, w2); }
    if (w3) { wcscat(context, L" "); wcscat(context, w3); }
    if (w4) { wcscat(context, L" "); wcscat(context, w4); }
    

    ngramTrieNode *startNode = traverseContext(root, context);
    if (!startNode) return NULL;

    Suggestion suggestions[MAX_RESULTS];
    wchar_t buffer[MAX_PHRASE_LEN];
    *count = 0;
    wmemset(buffer, 0, MAX_PHRASE_LEN);
    collectSuggestions(startNode, buffer, 0, suggestions, count);

    qsort(suggestions, *count, sizeof(Suggestion), compareSuggestions);
    
    wchar_t **results = malloc(sizeof(wchar_t *) * (*count));
    for (int i = 0; i < *count; i++) {
    	size_t inputLen = wcslen(context);  
    	size_t suggLen = wcslen(suggestions[i].phrase);
    	results[i] = malloc(sizeof(wchar_t) * (inputLen + suggLen + 1));
    	wcscpy(results[i], context);                         // Copy input word
    	wcscat(results[i], suggestions[i].phrase);      // Directly append suggestion 
    }

    return results;
}

// Create a new Trie Node
ngramTrieNode* createNgramNode() {
    ngramTrieNode *node = (ngramTrieNode*)malloc(sizeof(ngramTrieNode));
    node->isEndOfWord = 0;
    node->frequency = 0;
    for (int i = 0; i <= MAX_DEVA_CHARS; i++)
        node->children[i] = NULL;
    return node;
}

// Insert N-gram into Trie (preserve spaces)
void insertNgram(ngramTrieNode *root, const wchar_t *ngram) {
    ngramTrieNode *current = root;
    const wchar_t *p = ngram;

    while (*p) {
        int offset;

        if (*p == L' ') {
            offset = 0;  // Space gets index 0
        } else {
            offset = (*p - UNICODE_BASE) + 1;  // Shift Devanagari to index 1+
            if (offset <= 0 || offset > MAX_DEVA_CHARS) {
                p++;  // Skip unsupported characters
                continue;
            }
        }

        if (current->children[offset] == NULL)
            current->children[offset] = createNgramNode();

        current = current->children[offset];
        p++;
    }

    current->isEndOfWord = 1;
    current->frequency += 1;
}

// Display Trie 
void displayNgramTrie(ngramTrieNode *root, wchar_t *buffer, int depth) {
    if (root->isEndOfWord) {
        buffer[depth] = L'\0';
        wprintf(L"Ngram: %ls | Frequency: %d\n", buffer, root->frequency);
    }

    for (int i = 0; i <= MAX_DEVA_CHARS; i++) {
        if (root->children[i]) {
            if (i == 0)
                buffer[depth] = L' ';  // Index 0 → space
            else
                buffer[depth] = (wchar_t)(UNICODE_BASE + i - 1);

            displayNgramTrie(root->children[i], buffer, depth + 1);
        }
    }
}

// Free memory
void freeNgramTrie(ngramTrieNode *root) {
    for (int i = 0; i <= MAX_DEVA_CHARS; i++) {
        if (root->children[i])
            freeNgramTrie(root->children[i]);
    }
    free(root);
}

ngramTrieNode *buildNgramTrie(const char *filepath) {
   	
       	setlocale(LC_ALL, "");

        ngramTrieNode *root = createNgramNode();
        wchar_t buffer[MAX_NGRAM_LEN];
        FILE *file = fopen(filepath, "r, ccs=UTF-8");

        if (file == NULL) {
            perror("Error opening ngram file");
            exit(1);
        }

        wchar_t line[MAX_NGRAM_LEN];
        while (fgetws(line, sizeof(line) / sizeof(wchar_t), file)) {
            // Clean newline chars
            wchar_t *pos;
            if ((pos = wcschr(line, L'\n')) != NULL) *pos = L'\0';
            if ((pos = wcschr(line, L'\r')) != NULL) *pos = L'\0';

            if (wcslen(line) > 0 && wcscspn(line, L"\x00-\x08\x0B\x0C\x0E-\x1F") == wcslen(line)) {
                insertNgram(root, line);
            }
        }

        fclose(file);

        wprintf(L"Ngrams in File: %s \n", filepath);
        //displayNgramTrie(root, buffer, 0);
    	return (root);
}

