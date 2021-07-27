#if BPI == 16
#define ODD_MASK  0xaaaa
#define EVEN_MASK 0x5555
#else
#define ODD_MASK  0xaaaaaaaa
#define EVEN_MASK 0x55555555
#endif

#define POSITIVE 1
#define NEGATIVE 0

#define PRESENT 1
#define ABSENT  0

#define RAISED 2

typedef struct {
    int variable;
    int free_count;
} VAR;

/* black_white.c */
void merge_list();
void print_bw(int size);
void push_black_list();
void pop_black_list();
void setup_bw(pset_family R, pset c);
void free_bw();
int black_white();
void reset_black_list();
void split_list(pset_family R, int v);
void variable_list_alloc(int size);
void variable_list_init(int reduced_c_free_count, int *reduced_c_free_list);
void variable_list_delete(int element);
void variable_list_insert(int element);
int variable_list_empty();
void get_next_variable(int *pv, int *pphase, pset_family R);
void print_variable_list();
/* canonical.c */
pset_family find_canonical_cover(pset_family F1, pset_family D, pset_family R);
/* essentiality.c */
pset_family etr_order(pset_family F, pset_family E, pset_family R, pset c,
                      pset d);
int ascending(VAR *p1, VAR *p2);
void aux_etr_order(pset_family F, pset_family E, pset_family R, pset c, pset d);
pset_family get_mins(pset c);
void print_list(int n, int *x, char *name);
/* util_signature.c */
void set_time_limit(int seconds);
void print_cover(pset_family F, char *name);
int sf_equal(pset_family F1, pset_family F2);
int time_usage(char *name);
void s_totals(long time, int i);
void s_runtime(long total);
/* sigma.c */
pset get_sigma(pset_family R, pset c);
void set_not(pset c);
/* signature.c */
pset_family signature(pset_family F1, pset_family D1, pset_family R1);
pset_family generate_primes(pset_family F, pset_family R);
void cleanup();
/* signature_exact.c */
pset_family signature_minimize_exact(pset_family ESCubes, pset_family ESSet);
sm_matrix *signature_form_table(pset_family ESCubes, pset_family ESSet);
