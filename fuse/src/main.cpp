#if (defined(_WIN32) || defined(_WIN64))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__
#include <winsock2.h>
#include <Windows.h>
#include <time.h>
#define usleep(t) Sleep((DWORD)(t)/1000)
#define sleep(t)  Sleep((DWORD)(t)*1000)
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <mythcontrol.h>
#include <mytheventhandler.h>
#include <mythdebug.h>

#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <fuse/fuse_opt.h>

#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <signal.h>
#include <map>
#include <list>
#include <set>
#include <string>
#include <iterator>
#include <string.h>
#include <mutex>

#define MYTAG "[MYTHTVFS] "
#define DEBUG

static Myth::Control *mythtv = NULL;
static Myth::EventHandler *event_handler = NULL;

//// FUSE setup and options

struct mythtvfs_options
{
  mythtvfs_options() : displayHelp(0),
                       showVersion(0), backend((char*)"127.0.0.1"), protoPort(6543), wsapiPort(6544), recordings(NULL), pin((char*)"0000") {}

  int displayHelp;
  int showVersion;
  char* recordings;
  char* backend;
  int protoPort;
  int wsapiPort;
  char *pin;
};

static struct fuse_opt mythtvfs_opts[] = {
  {"-h", offsetof(struct mythtvfs_options, displayHelp), 1},
  {"--help", offsetof(struct mythtvfs_options, displayHelp), 1},
  {"--backend=%s", offsetof(struct mythtvfs_options, backend),0},
  {"--port=%i", offsetof(struct mythtvfs_options, protoPort), 0},
  {"--wsport=%i", offsetof(struct mythtvfs_options, wsapiPort), 0},
  {"--recordings=%s", offsetof(struct mythtvfs_options, recordings),0},
  {"-V", offsetof(struct mythtvfs_options, showVersion),1},
  {"--version", offsetof(struct mythtvfs_options, showVersion),1},
  FUSE_OPT_END
};

static struct fuse_operations mythtvfs_oper = {
  0,
};

static mythtvfs_options options;

//// Utility functions

class PathInfo {
public:
  PathInfo(const char *path) : show(NULL), episode(NULL) {
    if(strcmp(path, "/") != 0) {
      const char *sep = strstr(path + 1, "/");
      if(sep) {
        show = strndup(path + 1, sep - path - 1);
        episode = strdup(sep + 1);
      } else {
        show = strdup(path + 1);
      }
    }
  }

  ~PathInfo() {
    if(show) {
      free(show);
    }

    if(episode) {
      free(episode);
    }
  }

  char *show;
  char *episode;
};

//// Mythtv State

class MythRecording {
public:
  MythRecording(struct Myth::Program &program) {
    title = program.title;
    filename = program.fileName;

    std::size_t p = filename.find_last_of(".");
    subTitle = program.subTitle + filename.substr(p);
  }

  std::string title;
  std::string subTitle;
  std::string filename;
};


class MythtvState : public Myth::EventSubscriber {
public:
  std::list<MythRecording> shows;
  std::mutex state_lock;

  void fillShows() {
    std::lock_guard<std::mutex> guard(state_lock);
    shows.clear();

    Myth::ProgramListPtr progs = mythtv->GetRecordedList();

    int L = progs->size();
    for (size_t i = 0; i < L; ++i) {
      Myth::ProgramPtr prog = (*progs)[i];
      MythRecording *rec = new MythRecording(*prog);
      shows.push_back(*rec);
    }
  }

  //  void HandleRecordingListChange(const Myth::EventMessage& msg) {
  // }

  void HandleBackendMessage(Myth::EventMessagePtr msg) {
    switch (msg->event) {
    case Myth::EVENT_RECORDING_LIST_CHANGE:
      // got a change to the recordings

      break;
    }
  }

  void enumerateShows(void* buf, fuse_fill_dir_t filler) {
    std::lock_guard<std::mutex> guard(state_lock);

    std::set<std::string> seen;

    for(auto it : shows) {
      if(seen.count(it.title) == 0) {
        seen.insert(it.title);
        filler(buf, it.title.c_str(), NULL, 0);
      }
    }
  }

  int enumerateEpisodes(std::string show, void* buf, fuse_fill_dir_t filler) {
    std::lock_guard<std::mutex> guard(state_lock);

    for(auto it : shows) {
      if(it.title == show) {
        filler(buf, it.subTitle.c_str(), NULL, 0);
      }
    }
    return 0;
  }
};

static MythtvState *state = NULL;

//// FUSE operations

extern "C" int mythtvfs_getattr(const char* pathStr, struct stat* info) {
#ifdef DEBUG
  fprintf(stderr, MYTAG "DEBUG: mythtvfs_getattr - %s\n", pathStr);
#endif

  PathInfo path(pathStr);
  if(path.episode) {
    info->st_mode = S_IFLNK | 0444;
  } else {
    info->st_mode = S_IFDIR | 0555;
  }

  info->st_uid = 0;
  info->st_gid = 0;
  return 0;
}

extern "C" int mythtvfs_readdir(const char* pathStr, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
#ifdef DEBUG
  fprintf(stderr, MYTAG "DEBUG: mythtvfs_readdir - %s\n", pathStr);
#endif

  if(mythtv->IsOpen()) {
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    PathInfo path(pathStr);
    if(path.show) {
#ifdef DEBUG
  fprintf(stderr, MYTAG "DEBUG: mythtvfs_readdir - listing episodes for '%s'\n", path.show);
#endif
      return state->enumerateEpisodes(path.show, buf, filler);
    } else {
      // enumerate the shows
      state->enumerateShows(buf, filler);
    }

    return 0;
  }

  return -ENOTCONN;
}

extern "C" int mythtvfs_readlink(const char *pathStr, char *buf, size_t size) {
#ifdef DEBUG
  fprintf(stderr, MYTAG "DEBUG: mythtvfs_readlink - %s\n", pathStr);
#endif

  return 0;
}

extern "C" int mythtvfs_unlink(const char *pathStr) {
#ifdef DEBUG
  fprintf(stderr, MYTAG "DEBUG: mythtvfs_unlink - %s\n", pathStr);
#endif

  return 0;
}

int main(int argc, char** argv)
{
  int ret;
#ifdef __WINDOWS__
  //Initialize Winsock
  WSADATA wsaData;
  if ((ret = WSAStartup(MAKEWORD(2, 2), &wsaData)))
    return ret;
#endif /* __WINDOWS__ */

  mythtvfs_oper.getattr = mythtvfs_getattr;
  mythtvfs_oper.readdir = mythtvfs_readdir;
  mythtvfs_oper.readlink = mythtvfs_readlink;
  mythtvfs_oper.unlink = mythtvfs_unlink;

  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  if (fuse_opt_parse(&args, &options, mythtvfs_opts,0)==-1) {
    std::cerr << "Error parsing arguments" << std::endl;
    return -1;
  }

  if (options.displayHelp) {
    fuse_opt_add_arg(&args, "-h");
  } else if (options.showVersion) {
    fuse_opt_add_arg(&args, "-V");
  }

  if(options.recordings) {
    mythtv = new Myth::Control(options.backend, options.protoPort, options.wsapiPort, options.pin);
    if(!mythtv->Open()) {
      fprintf(stderr, MYTAG "ERROR: can not connect to the MythTV backend %s:%d\n", options.backend, options.protoPort);
      return -1;
    }

    event_handler = new Myth::EventHandler(options.backend, options.protoPort);
    state = new MythtvState();
    state->fillShows();
  } else {
    fprintf(stderr, MYTAG "USAGE: %s --recordings {path to mythtv recordings}\n", argv[0]);
    return -1;
  }

  ret = fuse_main(args.argc, args.argv, &mythtvfs_oper, NULL);
  if (options.displayHelp)
    {
      std::cout << std::endl << "mythtvfs options:" << std::endl;
      std::cout << "    --backend=<host>     The IP address of the MythTV back end to connect to"<< std::endl;
      std::cout << "    --port=<port>        The port the MythTV back end is listening on"<< std::endl;
      std::cout << "    --recordings=<path>  The path containing MythTV recordings"<< std::endl;
    }

#ifdef __WINDOWS__
  WSACleanup();
#endif /* __WINDOWS__ */
  return ret;
}
