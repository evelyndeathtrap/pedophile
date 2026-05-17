#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE 4096
#define MAX_RESPONSE_LEN 256

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
    
    // Dynamic Runtime State Variables for Activation Spreading
    double activation; 
    int recently_used;
} NeuronNode;

typedef struct {
    NeuronNode *buckets[HASH_SIZE];
    int total_nodes;
} AdvancedNetwork;

// --- Core Declarations ---
AdvancedNetwork* create_network();
NeuronNode* find_node(AdvancedNetwork *net, const char *id);
NeuronNode* add_word_node(AdvancedNetwork *net, const char *word);
void adjust_connection(NeuronNode *from, NeuronNode *to, double weight_delta);
void train_on_text(AdvancedNetwork *net, const char *text, double scaling_factor);
void save_model(AdvancedNetwork *net, const char *filename);
AdvancedNetwork* load_model(const char *filename);
void free_network(AdvancedNetwork *net);
char* read_file_to_string(const char *filename);

// --- Advanced Activation Routing Declarations ---
void generate_coherent_retort(AdvancedNetwork *net, const char *input_phrase);
void reset_network_energy(AdvancedNetwork *net);
void spread_activation(AdvancedNetwork *net, NeuronNode *source, double energy);

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
                // Bi-directional wiring to create conceptual feedback loops
                adjust_connection(prev, current, scaling_factor * 1.5);
                adjust_connection(current, prev, scaling_factor * 0.8);
            }
            prev = current;
        }
        free(clean);
        token = strtok(NULL, " \n\r\t");
    }
    free(text_copy);
}

// Clear electrical state charges across the neural net heap
void reset_network_energy(AdvancedNetwork *net) {
    for (int i = 0; i < HASH_SIZE; i++) {
        NeuronNode *curr = net->buckets[i];
        while (curr != NULL) {
            curr->activation = 0.0;
            curr->recently_used = 0;
            curr = curr->next_in_bucket;
        }
    }
}

// Recurse energy outward through the network map topology 
void spread_activation(AdvancedNetwork *net, NeuronNode *source, double energy) {
    if (!source || energy < 0.05) return; // Cut-off decay baseline

    ConnectionNode *conn = source->connections;
    while (conn != NULL) {
        // Accumulate mathematical charge based on synapse connection durability
        conn->target_neuron->activation += energy * (conn->weight * 0.3);
        
        // Spread decay further into the mesh
        spread_activation(net, conn->target_neuron, energy * 0.2);
        conn = conn->next;
    }
}

// Generate an active context retort matching the collective input phrase
void generate_coherent_retort(AdvancedNetwork *net, const char *input_phrase) {
    reset_network_energy(net);

    char *phrase_copy = strdup(input_phrase);
    char *token = strtok(phrase_copy, " \n\r\t");
    NeuronNode *seed_nodes[50];
    int seed_count = 0;

    // 1. Fire up the entire input context matrix simultaneously
    while (token != NULL && seed_count < 50) {
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
                seed_nodes[seed_count++] = node;
                node->recently_used = 1; // Don't parrot the input immediately
            }
        }
        free(clean);
        token = strtok(NULL, " \n\r\t");
    }
    free(phrase_copy);

    if (seed_count == 0) {
        printf("\n[Retort Engine]: ... (No context understood to reply to)\n");
        return;
    }

    // Spread activation energy from all seed concept vectors
    for (int i = 0; i < seed_count; i++) {
        spread_activation(net, seed_nodes[i], 1.0);
    }

    printf("\n[Input statement]: %s", input_phrase);
    printf("\n[Retort Response]: ");

    // 2. Select the next word sequence based on aggregate contextual charge
    NeuronNode *curr = NULL;
    int words_outputted = 0;

    while (words_outputted < MAX_RESPONSE_LEN) {
        NeuronNode *best_candidate = NULL;
        double highest_charge = -1.0;

        // If we are starting, choose the highest charged node anywhere in the field
        if (curr == NULL) {
            for (int i = 0; i < HASH_SIZE; i++) {
                NeuronNode *n = net->buckets[i];
                while (n != NULL) {
                    if (!n->recently_used && n->activation > highest_charge) {
                        highest_charge = n->activation;
                        best_candidate = n;
                    }
                    n = n->next_in_bucket;
                }
            }
        } else {
            // If inside a sentence trace, score candidate words by combined spatial link + current global charge
            ConnectionNode *conn = curr->connections;
            while (conn != NULL) {
                double contextual_score = conn->weight + (conn->target_neuron->activation * 0.5);
                if (!conn->target_neuron->recently_used && contextual_score > highest_charge) {
                    highest_charge = contextual_score;
                    best_candidate = conn->target_neuron;
                }
                conn = conn->next;
            }
        }

        // Output and shift frame context if a valid next word exists
        if (best_candidate && highest_charge > 0.0) {
            printf("%s ", best_candidate->identifier);
            best_candidate->recently_used = 1; // Prevent loops/infinite repetition
            curr = best_candidate;
            words_outputted++;
        } else {
            break; // Retort successfully resolved structure bounds
        }
    }
    printf("\n");
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
    const char *model_path = "retort_brain.model";
    if (argc < 3) {
        printf("Usage Options:\n  %s --train <file.txt>\n  %s --generate \"input query statement\"\n", argv[0], argv[0]);
        return 1;
    }

    AdvancedNetwork *net = load_model(model_path);

    if (strcmp(argv[1], "--train") == 0) {
        char *contents = read_file_to_string(argv[2]);
        if (contents) {
            train_on_text(net, contents, 1.0);
            save_model(net, model_path);
            printf("Training sequence processing complete. Core saved.\n");
            free(contents);
        }
    } else if (strcmp(argv[1], "--generate") == 0) {
        generate_coherent_retort(net, argv[2]);
    }

    free_network(net);
    return 0;
}
