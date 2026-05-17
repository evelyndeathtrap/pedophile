#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_TOKENS 2000
#define TOKEN_LEN 64
#define CMD_LEN 256
#define THRESHOLD 3
#define MEMORY_DEPTH 6  // Tracks the last 6 tokens to prevent localized looping

typedef struct {
    char token[TOKEN_LEN];
    int weight;
    char command[CMD_LEN];
} Node;

typedef struct {
    int from_idx;
    int to_idx;
    int weight;
} Transition;

Node network[MAX_TOKENS];
int network_size = 0;

Transition transitions[MAX_TOKENS * 4];
int transition_count = 0;

// Tabu Memory List to track what was recently generated
int recent_memory[MEMORY_DEPTH];
int memory_head = 0;

void add_to_memory(int idx) {
    recent_memory[memory_head] = idx;
    memory_head = (memory_head + 1) % MEMORY_DEPTH;
}

int is_in_memory(int idx) {
    for (int i = 0; i < MEMORY_DEPTH; i++) {
        if (recent_memory[i] == idx) return 1;
    }
    return 0;
}

int get_or_create_token(const char *token_str) {
    for (int i = 0; i < network_size; i++) {
        if (strcmp(network[i].token, token_str) == 0) {
            return i;
        }
    }
    if (network_size >= MAX_TOKENS) return -1;
    
    strncpy(network[network_size].token, token_str, TOKEN_LEN);
    network[network_size].weight = 0;
    memset(network[network_size].command, 0, CMD_LEN);
    network_size++;
    return network_size - 1;
}

void record_transition(int from_idx, int to_idx) {
    for (int i = 0; i < transition_count; i++) {
        if (transitions[i].from_idx == from_idx && transitions[i].to_idx == to_idx) {
            transitions[i].weight++;
            return;
        }
    }
    if (transition_count < (MAX_TOKENS * 4)) {
        transitions[transition_count].from_idx = from_idx;
        transitions[transition_count].to_idx = to_idx;
        transitions[transition_count].weight = 1;
        transition_count++;
    }
}

void save_brain() {
    FILE *f = fopen("brain.dat", "wb");
    if (!f) return;
    fwrite(&network_size, sizeof(int), 1, f);
    fwrite(network, sizeof(Node), network_size, f);
    fwrite(&transition_count, sizeof(int), 1, f);
    fwrite(transitions, sizeof(Transition), transition_count, f);
    fclose(f);
}

void load_brain() {
    FILE *f = fopen("brain.dat", "rb");
    if (!f) return;
    if (fread(&network_size, sizeof(int), 1, f) == 1) {
        fread(network, sizeof(Node), network_size, f);
    }
    if (fread(&transition_count, sizeof(int), 1, f) == 1) {
        fread(transitions, sizeof(Transition), transition_count, f);
    }
    fclose(f);
}

int tokenize_input(const char *input, char tokens[][TOKEN_LEN], int max_expected) {
    int count = 0;
    int i = 0;
    int len = strlen(input);

    while (i < len && count < max_expected) {
        if (input[i] == '\n') {
            strcpy(tokens[count++], "\n");
            i++;
        } else if (isspace(input[i])) {
            int t = 0;
            while (i < len && isspace(input[i]) && input[i] != '\n' && t < TOKEN_LEN - 1) {
                tokens[count][t++] = input[i++];
            }
            tokens[count][t] = '\0';
            count++;
        } else if (isalnum(input[i])) {
            int t = 0;
            while (i < len && isalnum(input[i]) && t < TOKEN_LEN - 1) {
                tokens[count][t++] = input[i++];
            }
            tokens[count][t] = '\0';
            count++;
        } else {
            tokens[count][0] = input[i];
            tokens[count][1] = '\0';
            count++;
            i++;
        }
    }
    return count;
}

void train_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *file_buffer = malloc(file_size + 1);
    if (!file_buffer) { fclose(f); return; }
    fread(file_buffer, 1, file_size, f);
    file_buffer[file_size] = '\0';
    fclose(f);

    int max_possible_tokens = file_size; 
    char (*tokens)[TOKEN_LEN] = malloc(max_possible_tokens * sizeof(*tokens));
    if (!tokens) { free(file_buffer); return; }

    int total_tokens = tokenize_input(file_buffer, tokens, max_possible_tokens);
    int last_idx = -1;

    printf("=== TRAINING MODE ===\n[Parsing %d tokens...]\n", total_tokens);

    for (int i = 0; i < total_tokens; i++) {
        int idx = get_or_create_token(tokens[i]);
        if (idx == -1) continue;

        network[idx].weight++;

        if (last_idx != -1) {
            record_transition(last_idx, idx);
        }
        last_idx = idx;

        if (network[idx].weight < THRESHOLD) {
            char printable[TOKEN_LEN];
            strcpy(printable, network[idx].token);
            if (strcmp(printable, "\n") == 0) strcpy(printable, "\\n");

            printf("\n  ⚠️ [Anomaly]: '%s' (Weight %d/%d)\n", printable, network[idx].weight, THRESHOLD);
            printf("   ? Map system() command? (y/n): ");
            
            char choice = getchar();
            while (getchar() != '\n'); 

            if (choice == 'y' || choice == 'Y') {
                printf("   ? Enter command: ");
                if (fgets(network[idx].command, CMD_LEN, stdin)) {
                    network[idx].command[strcspn(network[idx].command, "\n")] = 0;
                }
            }
        } else {
            if (strlen(network[idx].command) > 0) {
                system(network[idx].command);
            }
        }
    }

    free(tokens);
    free(file_buffer);
    save_brain();
    printf("\n=== Brain Wired & Saved ===\n");
}

void generate_mode() {
    if (network_size == 0) {
        printf("The network is empty. Train it first.\n");
        return;
    }

    // Initialize recent memory with invalid values
    for (int i = 0; i < MEMORY_DEPTH; i++) recent_memory[i] = -1;

    int current_idx = rand() % network_size;
    int max_tokens = 60;

    for (int t = 0; t < max_tokens; t++) {
        // Output token text
        printf("%s", network[current_idx].token);
        add_to_memory(current_idx);

        int candidates[MAX_TOKENS];
        int candidate_weights[MAX_TOKENS];
        int total_weight = 0;
        int match_count = 0;

        // Collect prospective next nodes
        for (int i = 0; i < transition_count; i++) {
            if (transitions[i].from_idx == current_idx) {
                int possible_next = transitions[i].to_idx;
                
                // ANTI-REPETITION FILTER:
                // Skip this path entirely if it leads to a recently printed token.
                if (is_in_memory(possible_next)) {
                    continue; 
                }

                candidates[match_count] = possible_next;
                candidate_weights[match_count] = transitions[i].weight;
                total_weight += transitions[i].weight;
                match_count++;
            }
        }

        if (match_count > 0 && total_weight > 0) {
            int roll = rand() % total_weight;
            int current_sum = 0;
            for (int i = 0; i < match_count; i++) {
                current_sum += candidate_weights[i];
                if (roll < current_sum) {
                    current_idx = candidates[i];
                    break;
                }
            }
        } else {
            // Force a hard jump to a random token not in recent memory to break loop dead-ends
            int attempts = 0;
            do {
                current_idx = rand() % network_size;
                attempts++;
            } while (is_in_memory(current_idx) && attempts < 50);
        }
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    load_brain();

    if (argc > 1 && strcmp(argv[1], "--train") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s --train <filename>\n", argv[0]);
            return 1;
        }
        train_from_file(argv[2]);
    } else {
        generate_mode();
    }

    return 0;
}
