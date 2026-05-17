#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#define HASH_SIZE 4096
#define MAX_RESPONSE_LEN 100

struct NeuronNode;

typedef struct ConnectionNode {
    struct NeuronNode *target_neuron;
    double weight;
    struct ConnectionNode *next;
} ConnectionNode;

typedef struct NeuronNode {
    char *identifier;
    ConnectionNode *connections;
    struct NeuronNode *next_in_bucket;
    
    // Dynamic Activation accumulators
    double correlation_energy; 
    int is_exhausted;
} NeuronNode;

typedef struct {
    NeuronNode *buckets[HASH_SIZE];
    int total_nodes;
} AdvancedNetwork;

// --- Engine Prototypes ---
AdvancedNetwork* create_network();
NeuronNode* find_node(AdvancedNetwork *net, const char *id);
NeuronNode* add_word_node(AdvancedNetwork *net, const char *word);
void adjust_connection(NeuronNode *from, NeuronNode *to, double weight_delta);
void train_on_text(AdvancedNetwork *net, const char *text, double scaling_factor);
void save_model(AdvancedNetwork *net, const char *filename);
AdvancedNetwork* load_model(const char *filename);
void free_network(AdvancedNetwork *net);
char* read_file_to_string(const char *filename);

// --- Chaos and Probabilistic Stream Functions ---
void stream_stochastic_retort(AdvancedNetwork *net, const char *input_phrase);
void reset_runtime_states(AdvancedNetwork *net);
void propagate_correlation(AdvancedNetwork *net, NeuronNode *source, double energy, int depth);
unsigned int calculate_phrase_byte_hash(const char *phrase);

unsigned int hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) hash = ((hash << 5) + hash) + c;
    return hash % HASH_SIZE;
}

AdvancedNetwork* create_network() {
    return (AdvancedNetwork*)calloc(1, sizeof(AdvancedNetwork));
}

NeuronNode* find_node(AdvancedNetwork *net, const char *id) {
    unsigned int index = hash_string(id);
    NeuronNode *current = net->buckets[index];
    while (current != NULL) {
        if (strcmp(current->identifier, id) == 0) return current;
        current = current->next_in_bucket;
    }
    return NULL;
}

NeuronNode* add_word_node(AdvancedNetwork *net, const char *word) {
    NeuronNode *existing = find_node(net, word);
    if (existing) return existing;

    unsigned int index = hash_string(word);
    NeuronNode *new_node = (NeuronNode*)calloc(1, sizeof(NeuronNode));
    new_node->identifier = strdup(word);
    new_node->next_in_bucket = net->buckets[index];
    net->buckets[index] = new_node;
    net->total_nodes++;
    return new_node;
}

void adjust_connection(NeuronNode *from, NeuronNode *to, double weight_delta) {
    if (from == to) return;
    ConnectionNode *curr = from->connections;
    while (curr != NULL) {
        if (curr->target_neuron == to) {
            curr->weight += weight_delta;
            return;
        }
        curr = curr->next;
    }
    ConnectionNode *new_conn = (ConnectionNode*)malloc(sizeof(ConnectionNode));
    new_conn->target_neuron = to;
    new_conn->weight = weight_delta;
    new_conn->next = from->connections;
    from->connections = new_conn;
}

void train_on_text(AdvancedNetwork *net, const char *text, double scaling_factor) {
    char *text_copy = strdup(text);
    char *token = strtok(text_copy, " \n\r\t");
    NeuronNode *prev = NULL;

    while (token != NULL) {
        int len = strlen(token);
        char *clean = malloc(len + 1);
        int j = 0;
        for (int i = 0; token[i] != '\0'; i++) {
            if (isalnum((unsigned char)token[i])) clean[j++] = tolower((unsigned char)token[i]);
        }
        clean[j] = '\0';

        if (j > 0) {
            NeuronNode *current = add_word_node(net, clean);
            if (prev != NULL) {
                adjust_connection(prev, current, scaling_factor * 2.0);
                adjust_connection(current, prev, scaling_factor * 0.5);
            }
            prev = current;
        }
        free(clean);
        token = strtok(NULL, " \n\r\t");
    }
    free(text_copy);
}

void reset_runtime_states(AdvancedNetwork *net) {
    for (int i = 0; i < HASH_SIZE; i++) {
        NeuronNode *curr = net->buckets[i];
        while (curr != NULL) {
            curr->correlation_energy = 0.0;
            curr->is_exhausted = 0;
            curr = curr->next_in_bucket;
        }
    }
}

void propagate_correlation(AdvancedNetwork *net, NeuronNode *source, double energy, int depth) {
    if (!source || energy < 0.1 || depth > 3) return;
    ConnectionNode *conn = source->connections;
    while (conn != NULL) {
        conn->target_neuron->correlation_energy += energy * (conn->weight * 0.4);
        propagate_correlation(net, conn->target_neuron, energy * 0.25, depth + 1);
        conn = conn->next;
    }
}

// Factors in every single byte of the input text to alter the math seed
unsigned int calculate_phrase_byte_hash(const char *phrase) {
    unsigned int hash = 0xAAAAAAAA;
    while (*phrase) {
        hash ^= ((hash << 5) + (*phrase++) + (hash >> 2));
    }
    return hash;
}

// Dynamic Stochastic Selection Loop
void stream_stochastic_retort(AdvancedNetwork *net, const char *input_phrase) {
    reset_runtime_states(net);

    // Seed the system using both the system clock AND a precise byte hash of the input string
    unsigned int byte_entropy = calculate_phrase_byte_hash(input_phrase);
    srand(time(NULL) ^ byte_entropy);

    char *phrase_copy = strdup(input_phrase);
    char *token = strtok(phrase_copy, " \n\r\t");
    NeuronNode *input_neurons[64];
    int input_count = 0;

    while (token != NULL && input_count < 64) {
        int len = strlen(token);
        char *clean = malloc(len + 1);
        int j = 0;
        for (int i = 0; token[i] != '\0'; i++) {
            if (isalnum((unsigned char)token[i])) clean[j++] = tolower((unsigned char)token[i]);
        }
        clean[j] = '\0';

        if (j > 0) {
            NeuronNode *node = find_node(net, clean);
            if (node) {
                input_neurons[input_count++] = node;
                node->is_exhausted = 1; 
            }
        }
        free(clean);
        token = strtok(NULL, " \n\r\t");
    }
    free(phrase_copy);

    if (input_count == 0) {
        printf("[Engine]: ... (Unmapped system state)\n");
        return;
    }

    // Flood network fields
    for (int i = 0; i < input_count; i++) {
        propagate_correlation(net, input_neurons[i], 1.5, 0);
    }

    printf("\n[Input]: %s", input_phrase);
    printf("\n[Retort]: ");
    fflush(stdout);

    NeuronNode *curr_focus = NULL;
    int words_streamed = 0;

    // Temporary storage arrays for probabilistic tracking arrays
    NeuronNode *candidates[512];
    double candidate_scores[512];

    while (words_streamed < MAX_RESPONSE_LEN) {
        int candidate_count = 0;
        double total_score_pool = 0.0;

        if (curr_focus == NULL) {
            // Step 1: Scan global vocabulary using correlation scores
            for (int i = 0; i < HASH_SIZE; i++) {
                NeuronNode *n = net->buckets[i];
                while (n != NULL && candidate_count < 512) {
                    if (!n->is_exhausted && n->correlation_energy > 0.0) {
                        candidates[candidate_count] = n;
                        // Inject dynamic variance scaled by input entropy bytes
                        double score = n->correlation_energy + ((double)(rand() % 100) / 1000.0);
                        candidate_scores[candidate_count] = score;
                        total_score_pool += score;
                        candidate_count++;
                    }
                    n = n->next_in_bucket;
                }
            }
        } else {
            // Step 2: Scan relational connections branching off the current word focus
            ConnectionNode *conn = curr_focus->connections;
            while (conn != NULL && candidate_count < 512) {
                NeuronNode *candidate = conn->target_neuron;
                if (!candidate->is_exhausted) {
                    candidates[candidate_count] = candidate;
                    
                    // Math formula: Relation * Correlation + minor entropy variance
                    double base_score = conn->weight * (1.0 + candidate->correlation_energy);
                    double entropy_offset = ((double)(rand() % 100) / 500.0); 
                    
                    candidate_scores[candidate_count] = base_score + entropy_offset;
                    total_score_pool += candidate_scores[candidate_count];
                    candidate_count++;
                }
                conn = conn->next;
            }
        }

        // Stochastic Selection: Spin a weighted probability wheel
        if (candidate_count > 0 && total_score_pool > 0.0) {
            double random_point = ((double)rand() / (double)RAND_MAX) * total_score_pool;
            double rolling_sum = 0.0;
            NeuronNode *selected_word = NULL;

            for (int i = 0; i < candidate_count; i++) {
                rolling_sum += candidate_scores[i];
                if (random_point <= rolling_sum) {
                    selected_word = candidates[i];
                    break;
                }
            }

            if (!selected_word) selected_word = candidates[0]; // Fallback safety

            // Stream chosen token to user terminal
            printf("%s ", selected_word->identifier);
            fflush(stdout);
            
            selected_word->is_exhausted = 1; 
            curr_focus = selected_word;
            words_streamed++;
            
            usleep(60000); 
        } else {
            break; // Network safely resolved and terminated connection threads
        }
    }
    printf("\n\n");
}

char* read_file_to_string(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(length + 1);
    if (buffer) {
        size_t read_elements = fread(buffer, 1, length, file);
        buffer[read_elements] = '\0';
    }
    fclose(file);
    return buffer;
}

void save_model(AdvancedNetwork *net, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    fprintf(f, "%d\n", net->total_nodes);
    for (int i = 0; i < HASH_SIZE; i++) {
        NeuronNode *curr = net->buckets[i];
        while (curr != NULL) {
            fprintf(f, "DEF:%s\n", curr->identifier);
            curr = curr->next_in_bucket;
        }
    }
    for (int i = 0; i < HASH_SIZE; i++) {
        NeuronNode *curr = net->buckets[i];
        while (curr != NULL) {
            ConnectionNode *conn = curr->connections;
            while (conn != NULL) {
                fprintf(f, "LINK:%s:%s:%f\n", curr->identifier, conn->target_neuron->identifier, conn->weight);
                conn = conn->next;
            }
            curr = curr->next_in_bucket;
        }
    }
    fclose(f);
}

AdvancedNetwork* load_model(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return create_network();

    AdvancedNetwork *net = create_network();
    int size = 0;
    if (fscanf(f, "%d\n", &size) != 1) { fclose(f); return net; }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), f)) {
        buffer[strcspn(buffer, "\n")] = 0;
        if (strncmp(buffer, "DEF:", 4) == 0) {
            add_word_node(net, buffer + 4);
        } else if (strncmp(buffer, "LINK:", 5) == 0) {
            char *from = buffer + 5;
            char *to = strchr(from, ':');
            if (to) {
                *to = '\0'; to++;
                char *weight_str = strchr(to, ':');
                if (weight_str) {
                    *weight_str = '\0'; weight_str++;
                    adjust_connection(find_node(net, from), find_node(net, to), atof(weight_str));
                }
            }
        }
    }
    fclose(f);
    return net;
}

void free_network(AdvancedNetwork *net) {
    for (int i = 0; i < HASH_SIZE; i++) {
        NeuronNode *curr = net->buckets[i];
        while (curr != NULL) {
            NeuronNode *next_node = curr->next_in_bucket;
            ConnectionNode *conn = curr->connections;
            while (conn != NULL) {
                ConnectionNode *next_conn = conn->next;
                free(conn);
                conn = next_conn;
            }
            free(curr->identifier);
            free(curr);
            curr = next_node;
        }
    }
    free(net);
}

int main(int argc, char *argv[]) {
    const char *model_path = "stochastic_brain.model";
    if (argc < 3) {
        printf("Commands:\n  %s --train <source.txt>\n  %s --generate \"input string\"\n", argv[0], argv[0]);
        return 1;
    }

    AdvancedNetwork *net = load_model(model_path);

    if (strcmp(argv[1], "--train") == 0) {
        char *data = read_file_to_string(argv[2]);
        if (data) {
            train_on_text(net, data, 1.0);
            save_model(net, model_path);
            printf("Training complete. Network layout saved.\n");
            free(data);
        }
    } else if (strcmp(argv[1], "--generate") == 0) {
        stream_stochastic_retort(net, argv[2]);
    }

    free_network(net);
    return 0;
}
