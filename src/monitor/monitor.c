#include <isa.h>
#include <checkpoint/cpt_env.h>
#include <checkpoint/profiling.h>
#include <memory/image_loader.h>
#include <memory/paddr.h>
#include <getopt.h>
#include <stdlib.h>
#include<signal.h>
#include<unistd.h>

#ifndef CONFIG_SHARE
void init_aligncheck();
void init_log(const char *log_file);
void init_mem();
void init_regex();
void init_wp_pool();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int batch_mode = false;
static int difftest_port = 1234;
char *max_instr = NULL;

static char *etrace_inst = NULL;
static char *etrace_data = NULL;

extern char *mapped_cpt_file;  // defined in paddr.c
extern bool map_image_as_output_cpt;

int is_batch_mode() { return batch_mode; }

static inline void welcome() {
  Log("Debug: \33[1;32m%s\33[0m", MUXDEF(CONFIG_DEBUG, "ON","OFF"));
  IFDEF(CONFIG_DEBUG, Log("If debug mode is on, a log file will be generated "
      "to record every instruction NEMU executes. This may lead to a large log file. "
      "If it is not necessary, you can turn it off in include/common.h.")
  );
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to \33[1;41m\33[1;33m%s\33[0m-NEMU!\n", str(__ISA__));
  printf("For help, type \"help\"\n");
}

void sig_handler(int signum) {
  if (signum == SIGINT) {
    Log("received SIGINT, mark manual cpt flag\n");
    if (wait_manual_oneshot_cpt) {
      recvd_manual_oneshot_cpt = true;
    } else if (wait_manual_uniform_cpt) {
      recvd_manual_uniform_cpt = true;
      reset_inst_counters();
    } else {
      panic("Received SIGINT when not waiting for it");
    }
  } else {
    panic("Unhandled signal: %i\n", signum);
  }
}

static inline int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"max-instr", required_argument, NULL, 'I'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},

    // path setting
    {"output-base-dir"    , required_argument, NULL, 'D'},
    {"workload-name"      , required_argument, NULL, 'w'},
    {"config-name"        , required_argument, NULL, 'C'},

    // restore cpt
    {"restore"            , no_argument      , NULL, 'c'},
    {"cpt-restorer"       , required_argument, NULL, 'r'},
    {"map-img-as-outcpt"  , no_argument      , NULL, 13},

    // take cpt
    {"simpoint-dir"       , required_argument, NULL, 'S'},
    {"uniform-cpt"        , no_argument      , NULL, 'u'},
    {"manual-oneshot-cpt" , no_argument      , NULL, 8},
    {"manual-uniform-cpt" , no_argument      , NULL, 9},
    {"cpt-interval"       , required_argument, NULL, 5},
    {"cpt-mmode"          , no_argument      , NULL, 7},
    {"map-cpt"            , required_argument, NULL, 10},

    // profiling
    {"simpoint-profile"   , no_argument      , NULL, 3},
    {"dont-skip-boot"     , no_argument      , NULL, 6},
    {"etrace-inst"        , required_argument, NULL, 11},
    {"etrace-data"        , required_argument, NULL, 12},

    // restore cpt
    {"cpt-id"             , required_argument, NULL, 4},

    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bI:hl:d:p:D:w:C:cr:S:u", table, NULL)) != -1) {
    switch (o) {
      case 'b': batch_mode = true; break;
      case 'I': max_instr = optarg; break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1: {
        img_file = optarg;
        if (ISDEF(CONFIG_MODE_USER)) {
          return optind - 1;
        } else {
          break;
        }
      }

      case 'D': output_base_dir = optarg; break;
      case 'w': workload_name = optarg; break;
      case 'C': config_name = optarg; break;

      case 'c':
        checkpoint_restoring = true;
        Log("Restoring from checkpoint");
        break;

      case 'r':
        restorer = optarg;
        break;
      case 13: {
        extern bool map_image_as_output_cpt;
        map_image_as_output_cpt = true;
        break;
      }

      case 'S':
        assert(profiling_state == NoProfiling);
        simpoints_dir = optarg;
        profiling_state = SimpointCheckpointing;
        checkpoint_taking = true;
        Log("Taking simpoint checkpoints");
        break;

      case 'u':
        checkpoint_taking = true;
        break;

      case 5: sscanf(optarg, "%lu", &checkpoint_interval); break;

      case 3:
        assert(profiling_state == NoProfiling);
        profiling_state = SimpointProfiling;
        Log("Doing Simpoint Profiling");
        break;
      case 11: etrace_inst = optarg; break;
      case 12: etrace_data = optarg; break;

      case 6:
        // start profiling/checkpointing right after boot,
        // instead of waiting for the pseudo inst to notify NEMU.
        profiling_started = true;
        break;

      case 7:
        Log("Force to take checkpoint on m mode. You should know what you are doing!");
        force_cpt_mmode = true;
        break;

      case 8:
        wait_manual_oneshot_cpt = true;
        // fall through
      case 9:
        if (!wait_manual_oneshot_cpt) {
          wait_manual_uniform_cpt = true;
        }
        Log("Manually take cpt by send signal");
        checkpoint_taking = true;
        if (signal(SIGINT, sig_handler) == SIG_ERR) {
          panic("Cannot catch SIGINT!\n");
        }
        break;
      case 10: { // map-cpt
        mapped_cpt_file = optarg;
        Log("Setting mapped_cpt_file to %s", mapped_cpt_file);
        break;
      }

      case 4: sscanf(optarg, "%d", &cpt_id); break;

      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-I,--max-instr          max number of instructions executed\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");

        printf("\t-D,--statdir=STAT_DIR   store simpoint bbv, cpts, and stats in STAT_DIR\n");
        printf("\t-w,--workload=WORKLOAD  the name of sub_dir of this run in STAT_DIR\n");
        printf("\t-C,--config=CONFIG      running configuration\n");

        printf("\t-c,--restore            restoring from CPT FILE\n");
        printf("\t-r,--cpt-restorer=R     binary of gcpt restorer\n");
        printf("\t--map-img-as-outcpt     map to image as output checkpoint, do not truncate it\n");

        printf("\t-S,--simpoint-dir=SIMPOINT_DIR   simpoints dir\n");
        printf("\t-u,--uniform-cpt        uniformly take cpt with fixed interval\n");
        printf("\t--cpt-interval=INTERVAL cpt interval: the profiling period for simpoint; the checkpoint interval for uniform cpt\n");
        printf("\t--cpt-mmode             force to take cpt in mmode, which might not work.\n");
        printf("\t--manual-oneshot-cpt    Manually take one-shot cpt by send signal.\n");
        printf("\t--manual-uniform-cpt    Manually take uniform cpt by send signal.\n");
        printf("\t--map-cpt               map to this file as pmem, which can be treated as a checkpoint.\n");

        printf("\t--simpoint-profile      simpoint profiling\n");
        printf("\t--dont-skip-boot        profiling/checkpoint immediately after boot\n");
        printf("\t--cpt-id                checkpoint id\n");
        printf("\t--etrace-inst           GEM5 elastic trace inst output file\n");
        printf("\t--etrace-data           GEM5 elastic trace data output file\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
#ifdef CONFIG_MODE_USER
  int user_argidx = parse_args(argc, argv);
#else
  parse_args(argc, argv);
#endif

  if (map_image_as_output_cpt) {
    assert(!mapped_cpt_file);
    mapped_cpt_file = img_file;
    checkpoint_restoring = true;
  }

  extern void init_path_manager();
  extern void simpoint_init();
  extern void init_serializer();
  extern void unserialize();

  bool output_features_enabled = checkpoint_taking || profiling_state == SimpointProfiling;
  if (output_features_enabled) {
    init_path_manager();
    simpoint_init();
    init_serializer();
  }

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Load the image to memory. This will overwrite the built-in image. */
#ifdef CONFIG_MODE_USER
  int user_argc = argc - user_argidx;
  char **user_argv = argv + user_argidx;
  void init_user(char *elfpath, int argc, char *argv[]);
  init_user(img_file, user_argc, user_argv);
#else
  /* Perform ISA dependent initialization. */
  init_isa();

  // when there is a gcpt[restorer], we put bbl after gcpt[restorer]
  uint64_t bbl_start;
  long img_size; // how large we should copy for difftest

  if (checkpoint_restoring) {
    // When restoring cpt, gcpt restorer from cmdline is optional,
    // because a gcpt already ships a restorer
    assert(img_file != NULL);

    img_size = CONFIG_MSIZE;
    bbl_start = CONFIG_MSIZE; // bbl size should never be used, let it crash if used

    if (map_image_as_output_cpt) {  // map_cpt is loaded in init_mem
      Log("Restoring with memory image cpt");
    } else {
      load_img(img_file, "Gcpt file form cmdline", RESET_VECTOR, 0);
    }
    if (restorer) {
      load_img(restorer, "Gcpt restorer form cmdline", RESET_VECTOR, 0xf00);
    }

  } else if (checkpoint_taking) {
    // boot: jump to restorer --> restorer jump to bbl
    assert(img_file != NULL);
    assert(restorer != NULL);

    bbl_start = RESET_VECTOR + CONFIG_BBL_OFFSET_WITH_CPT;

    long restorer_size = load_img(restorer, "Gcpt restorer form cmdline", RESET_VECTOR, 0xf00);
    long bbl_size = load_img(img_file, "image (bbl/bare metal app) from cmdline", bbl_start, 0);
    img_size = restorer_size + bbl_size;
  } else {
    if (restorer != NULL) {
      Log("You are providing a gcpt restorer without specify ``restoring cpt'' or ``taking cpt''! "
      "If you don't know what you are doing, this will corrupt your memory/program. "
      "If you want to take cpt or restore cpt, you must EXPLICITLY add corresponding options");
      panic("Providing cpt restorer without restoring cpt or taking cpt\n");
    }
    bbl_start = RESET_VECTOR;
    img_size = load_img(img_file, "image (bbl/bare metal app) from cmdline", bbl_start, 0);
  }

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize devices. */
  init_device();

  extern void init_tracer(const char *data_file, const char *inst_file);
  init_tracer(etrace_data, etrace_inst);
#endif

  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();

  /* Enable alignment checking for in a x86 host */
  init_aligncheck();

  /* Display welcome message. */
  welcome();
}
#endif
