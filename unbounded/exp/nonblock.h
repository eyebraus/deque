
// experiment functions
double timing_exp();
double throughput_exp();

// thread runner functions
void *timing_run(void *args_void);
void *throughput_run(void *args_void);
void *throughput_kill(void *args_void);

// other helpers
void output_results(double result);

