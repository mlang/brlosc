#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <brlapi.h>
#include <lo/lo.h>

void error(int, const char *, const char *);

lo_server oscServer = NULL;
lo_address oscTarget = NULL;
int done = 0;

int enter_handler(const char *path, const char *types, lo_arg ** argv,
                  int argc, lo_message msg, void *user_data)
{
  int tty = argv[0]->i;
  int retval;
  retval = brlapi_enterTtyMode(tty, NULL);
  if (retval != -1) {
    lo_send_from(oscTarget, oscServer, LO_TT_IMMEDIATE,
                 "/tty", "i", retval);
  } else {
    lo_send_from(oscTarget, oscServer, LO_TT_IMMEDIATE,
                 "/error", "s", brlapi_strerror(&brlapi_error));
    fprintf(stderr, "Error entering tty: %s\n",
            brlapi_strerror(&brlapi_error));
  }

  return 0;
}

int write_handler(const char *path, const char *types, lo_arg ** argv,
                  int argc, lo_message msg, void *user_data)
{
  int retval;
  switch (types[0]) {
  case LO_BLOB: {
    unsigned int x, y;
    brlapi_getDisplaySize(&x, &y);
    if (argv[0]->blob.size == x*y) {
      retval = brlapi_writeDots(&argv[0]->blob.data);
    } else {
      fprintf(stderr, "Unexpected blob size\n");
      return 0;
    }
    break;
  }
  case LO_STRING:
    retval = brlapi_writeText(BRLAPI_CURSOR_OFF, &argv[0]->s);
    break;
  }

  if (retval == -1) {
    lo_send_from(oscTarget, oscServer, LO_TT_IMMEDIATE,
                 "/error", "s", brlapi_strerror(&brlapi_error));
    fprintf(stderr, "Error writing text: %s\n",
            brlapi_strerror(&brlapi_error));
  }

  return 0;
}

int leave_handler(const char *path, const char *types, lo_arg ** argv,
                  int argc, lo_message msg, void *user_data)
{
  int retval = brlapi_leaveTtyMode();
  if (retval == -1) {
    lo_send_from(oscTarget, oscServer, LO_TT_IMMEDIATE,
                 "/error", "s", brlapi_strerror(&brlapi_error));
    fprintf(stderr, "Error leaving tty mode: %s\n",
            brlapi_strerror(&brlapi_error));
  }

  return 0;
}

int quit_handler(const char *path, const char *types, lo_arg ** argv,
                 int argc, lo_message msg, void *user_data)
{
  done = 1;

  return 0;
}

int main(int argc, char *argv[])
{
  oscTarget = lo_address_new(NULL, argc > 1? argv[1]: "57120");
  brlapi_connectionSettings_t brlapi = BRLAPI_SETTINGS_INITIALIZER;
  if (argc > 3) {
    brlapi.host = argv[3];
  }
  int brl_fd = brlapi_openConnection(&brlapi, &brlapi);
  if (brl_fd == -1) {
    fprintf(stderr, "Couldn't connect to BrlAPI at %s: %s\n",
            brlapi.host, brlapi_strerror(&brlapi_error));
    exit(1);
  }
 
  oscServer = lo_server_new(argc > 2? argv[2]: "27500", error);

  lo_send_from(oscTarget, oscServer, LO_TT_IMMEDIATE,
	       "/connected", NULL);

  lo_server_add_method(oscServer, "/enter", "i", enter_handler, NULL);
  lo_server_add_method(oscServer, "/write", "s", write_handler, NULL);
  lo_server_add_method(oscServer, "/write", "b", write_handler, NULL);
  lo_server_add_method(oscServer, "/leave", "", leave_handler, NULL);
  lo_server_add_method(oscServer, "/quit", "", quit_handler, NULL);

  int lo_fd = lo_server_get_socket_fd(oscServer);
  
  if (lo_fd < 0) {
    fprintf(stderr, "Unable to get lo server socket fd\n");
    exit(1);
  }

  struct pollfd fds[] = {
    { .fd = lo_fd, .events = POLLIN },
    { .fd = brl_fd, .events = POLLIN }
  };
  const nfds_t nfds = sizeof(fds)/sizeof(struct pollfd);
  int retval;
  do {
    retval = poll(&fds[0], nfds, -1);
    if (retval == -1) {
      perror("poll");
      exit(1);
    } else if (retval > 0) {
      int i;
      for (i = 0; i != nfds; ++i) {
        if (fds[i].revents) {
          if (fds[i].fd == lo_fd) {
            lo_server_recv_noblock(oscServer, 0);
          }
          if (fds[i].fd == brl_fd) {
            brlapi_keyCode_t code;
            while (brlapi_readKey(0, &code) == 1) {
              brlapi_describedKeyCode_t description;
              brlapi_describeKeyCode(code, &description);
              lo_send_from(oscTarget, oscServer, LO_TT_IMMEDIATE,
                           "/key", "ssi",
                           description.type, description.command,
                           description.argument);
            }
          }
        }
      }
    }
  } while (!done);
  brlapi_closeConnection();
}

void error(int num, const char *msg, const char *path)
{
  printf("liblo server error %d in path %s: %s\n", num, path, msg);
}





