#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#define HASH_SIZE 4096
#define DECAY 0.70
#define MAX_WEIGHT 10.0
#define PLASTICITY 0.80

typedef enum { M_BYTE, M_INT, M_WORD, M_CMD } MType;

struct Node;
typedef struct Conn { struct Node *to; double wt; struct Conn *next; } Conn;
typedef struct Node { 
    MType type; char *id; double nrg; Conn *conns; struct Node *next;
    union { unsigned char bval; int ival; char *sval; } pld;
} Node;

typedef struct { Node *buckets[HASH_SIZE]; Node *bytes[256]; } Net;

// Tracking array to guarantee words are never printed twice in a single response flight
char *printed_registry[1024];
int printed_count = 0;

Net* create_net() {
    Net *n = calloc(1, sizeof(Net));
    for (int i=0; i<256; i++) {
        n->bytes[i] = calloc(1, sizeof(Node)); n->bytes[i]->type = M_BYTE; 
        n->bytes[i]->pld.bval = i; n->bytes[i]->id = malloc(16); sprintf(n->bytes[i]->id, "B_%02X", i);
        n->buckets[i % HASH_SIZE] = n->bytes[i];
    }
    for (int i=0; i<255; i++) {
        Conn *c = calloc(1, sizeof(Conn)); c->to = n->bytes[i+1]; 
        c->wt = ((i >= 'a' && i < 'z') || (i >= 'A' && i < 'Z')) ? 6.0 : 1.5;
        c->next = n->bytes[i]->conns; n->bytes[i]->conns = c;
    }
    return n;
}

void apply_plasticity_decay(Net *n) {
    for (int i = 0; i < HASH_SIZE; i++)
        for (Node *curr = n->buckets[i]; curr; curr = curr->next)
            for (Conn *c = curr->conns; c; c = c->next) c->wt *= PLASTICITY;
}

Node* find_node(Net *n, const char *id, MType t) {
    Node *c = n->buckets[abs((int)id[0]) % HASH_SIZE];
    while (c) { if (c->type == t && strcmp(c->id, id) == 0) return c; c = c->next; }
    return NULL;
}

void link_nodes(Node *f, Node *t, double base_amt) {
    if (!f || !t || f == t) return;
    for (Conn *c = f->conns; c; c = c->next) {
        if (c->to == t) { c->wt += base_amt / (1.0 + (c->wt * 0.1)); if (c->wt > MAX_WEIGHT) c->wt = MAX_WEIGHT; return; }
    }
    Conn *nc = calloc(1, sizeof(Conn)); nc->to = t; nc->wt = base_amt * 2.5; 
    if (nc->wt > MAX_WEIGHT) nc->wt = MAX_WEIGHT; nc->next = f->conns; f->conns = nc;
}

int is_delimiter(char c, char next_c) {
    if (isspace((unsigned char)c) || ispunct((unsigned char)c)) return 1;
    if (isupper((unsigned char)next_c) && islower((unsigned char)c)) return 1; // Capitalization shift boundary
    return 0;
}

void train(Net *n, const char *txt) {
    int i = 0; Node *p_b = NULL, *p_m = NULL;
    while (txt[i]) {
        Node *c_b = n->bytes[(unsigned char)txt[i]];
        if (p_b) link_nodes(p_b, c_b, 1.5); p_b = c_b;

        if (isalnum((unsigned char)txt[i])) {
            int s = i; 
            while (txt[i] && isalnum((unsigned char)txt[i]) && !is_delimiter(txt[i], txt[i+1])) { i++; }
            if (s == i) i++; 
            
            char *tok = strndup(&txt[s], i - s);
            MType t = (strlen(tok) > 12) ? M_CMD : M_WORD; // Long text automatically becomes a shell system command payload
            
            Node *w = find_node(n, tok, t);
            if (!w) {
                w = calloc(1, sizeof(Node)); w->type = t; w->id = strdup(tok); w->pld.sval = w->id;
                w->next = n->buckets[abs((int)tok[0]) % HASH_SIZE]; n->buckets[abs((int)tok[0]) % HASH_SIZE] = w;
                link_nodes(w, n->bytes[(unsigned char)tok[0]], 1.0); link_nodes(n->bytes[(unsigned char)tok[0]], w, 1.5);
            }
            if (p_m) link_nodes(p_m, w, 2.5); p_m = w; free(tok); continue;
        }
        i++;
    }
}

void push(Net *n, Node *s, double e, int d) {
    if (!s || e < 0.05 || d > 3) return;
    for (Conn *c = s->conns; c; c = c->next) { c->to->nrg += e * (c->wt * 0.4); push(n, c->to, e * 0.2, d + 1); }
}

void inject(Net *n, const char *p) {
    for (int i=0; p[i]; i++) { n->bytes[(unsigned char)p[i]]->nrg += 20.0; push(n, n->bytes[(unsigned char)p[i]], 8.0, 0); }
    char *cp = strdup(p), *t = strtok(cp, " \t\n\r.,!?");
    while (t) { 
        Node *w = find_node(n, t, (strlen(t) > 12)?M_CMD:M_WORD); 
        if (w) { w->nrg += 25.0; push(n, w, 10.0, 0); } 
        t = strtok(NULL, " \t\n\r.,!?"); 
    }
    free(cp);
}

void run_cmd(Net *n, const char *cmd) {
    char input_args[256];
    printf("\n[SYSTEM SYSTEM() REQUESTED]: Enforced Command Context: %s\n", cmd);
    printf("Enter execution parameter options/arguments: ");
    if (!fgets(input_args, sizeof(input_args), stdin)) return;
    input_args[strcspn(input_args, "\n")] = 0;

    char combined_statement[512];
    snprintf(combined_statement, sizeof(combined_statement), "%s %s", cmd, input_args);
    
    printf("Executing statement: '%s'\n", combined_statement);
    FILE *f = popen(combined_statement, "r"); 
    if (!f) return;
    
    char b[128]; 
    while (fgets(b, sizeof(b), f)) { printf(" > %s", b); inject(n, b); } 
    pclose(f);
}

int has_been_printed(const char *id) {
    for (int i = 0; i < printed_count; i++) {
        if (strcmp(printed_registry[i], id) == 0) return 1;
    }
    return 0;
}

void respond(Net *n, const char *prompt, int steps) {
    for (int i=0; i<HASH_SIZE; i++) for (Node *c = n->buckets[i]; c; c = c->next) c->nrg = 0;
    printed_count = 0; 
    inject(n, prompt);
    printf("[Out]: ");
    
    Node *curr = n->bytes[(unsigned char)prompt[strlen(prompt)-1]];
    Node *cand[512]; double sc[512];
    
    for (int s=0; s<steps; s++) {
        int cc = 0; double tot = 0;
        if (!curr) {
            for (int i=0; i<HASH_SIZE; i++)
                for (Node *node = n->buckets[i]; node && cc<512; node = node->next) {
                    if (node->nrg > 0.05 && !has_been_printed(node->id)) { 
                        cand[cc] = node; sc[cc] = node->nrg; tot += node->nrg; cc++; 
                    }
                }
        } else {
            for (Conn *cn = curr->conns; cn && cc<512; cn = cn->next) {
                double score = cn->wt * (1.0 + cn->to->nrg);
                if (score > 0.05 && !has_been_printed(cn->to->id)) { 
                    cand[cc] = cn->to; sc[cc] = score; tot += score; cc++; 
                }
            }
        }
        if (!cc || tot <= 0) break;
        double r = ((double)rand() / RAND_MAX) * tot, sum = 0; Node *sel = NULL;
        for (int i=0; i<cc; i++) { sum += sc[i]; if (r <= sum) { sel = cand[i]; break; } }
        if (!sel) sel = cand[0];

        // Add to registry to prevent future repetitions
        printed_registry[printed_count++] = sel->id;

        switch (sel->type) {
            case M_BYTE: printf("%c", sel->pld.bval); break;
            case M_INT:  printf(" [INT:%d] ", sel->pld.ival); break;
            case M_WORD: printf("%s ", sel->pld.sval); break;
            case M_CMD:  run_cmd(n, sel->pld.sval); break;
        }
        fflush(stdout); curr = sel;
        for (int i=0; i<HASH_SIZE; i++) for (Node *d = n->buckets[i]; d; d = d->next) d->nrg *= DECAY;
    }
    printf("\n");
}

int main(int argc, char **argv) {
    srand(time(NULL)); Net *n = create_net();
    
    Node *ni = calloc(1, sizeof(Node)); ni->type = M_INT; ni->id = strdup("num"); ni->pld.ival = 100;
    ni->next = n->buckets[abs((int)ni->id[0]) % HASH_SIZE]; n->buckets[abs((int)ni->id[0]) % HASH_SIZE] = ni;
    link_nodes(n->bytes['#'], ni, 4.0);

    train(n, "Alpha progression, split.ByPunctuation elements... Newline\nTest validationStringCheck");

    if (argc > 2 && strcmp(argv[1], "--train") == 0) {
        FILE *f = fopen(argv[2], "rb");
        if (f) { 
            fseek(f,0,SEEK_END); long l=ftell(f); fseek(f,0,SEEK_SET); char *b=malloc(l+1); fread(b,1,l,f); b[l]='\0'; 
            apply_plasticity_decay(n); 
            train(n, b); 
            printf("[System]: Strict custom parsing training configuration set.\n");
            free(b); fclose(f); 
        }
    }
    
    respond(n, argc > 1 ? argv[argc-1] : "a", 25);
    return 0;
}
