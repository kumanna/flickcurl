/*
 * flickcurl - Invoke Flickr API via CURL
 *
 * USAGE: flickcurl OPTIONS command args
 *
 * To authenticate:
 *   flickcurl -a FROB
 * FROB like 123-456-789
 *
 * flickcurl photo-getinfo PHOTO-ID
 * PHOTO-ID like 123456789
 * This method does not require authentication.
 * -- http://www.flickr.com/services/api/flickr.photos.getInfo.html
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* many places for getopt */
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
#include <flickcurl_getopt.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <flickcurl.h>



#ifdef NEED_OPTIND_DECLARATION
extern int optind;
extern char *optarg;
#endif


static const char* program;

static const char*
my_basename(const char *name)
{
  char *p;
  if((p=strrchr(name, '/')))
    name=p+1;
  else if((p=strrchr(name, '\\')))
    name=p+1;

  return name;
}


static void
my_message_handler(void *user_data, const char *message)
{
  fprintf(stderr, "%s: ERROR: %s\n", program, message);
}


static void
my_set_config_var_handler(void* userdata, const char* key, const char* value)
{
  flickcurl *fc=(flickcurl *)userdata;
  
  if(!strcmp(key, "api_key"))
    flickcurl_set_api_key(fc, value);
  else if(!strcmp(key, "secret"))
    flickcurl_set_shared_secret(fc, value);
  else if(!strcmp(key, "auth_token"))
    flickcurl_set_auth_token(fc, value);
}


#ifdef HAVE_GETOPT_LONG
#define HELP_TEXT(short, long, description) "  -" short ", --" long "  " description
#define HELP_TEXT_LONG(long, description) "      --" long "  " description
#define HELP_ARG(short, long) "--" #long
#define HELP_PAD "\n                          "
#else
#define HELP_TEXT(short, long, description) "  -" short "  " description
#define HELP_TEXT_LONG(long, description)
#define HELP_ARG(short, long) "-" #short
#define HELP_PAD "\n      "
#endif


#define GETOPT_STRING "a:hv"

#ifdef HAVE_GETOPT_LONG
static struct option long_options[] =
{
  /* name, has_arg, flag, val */
  {"auth",    1, 0, 'a'},
  {"help",    0, 0, 'h'},
  {"version", 0, 0, 'v'},
  {NULL,      0, 0, 0}
};
#endif


typedef int (*command_handler)(flickcurl* fc, int argc, char *argv[]);


static int
command_test_echo(flickcurl* fc, int argc, char *argv[])
{
  return flickcurl_test_echo(fc, argv[1], argv[2]);
}


#if 0
static void command_flickcurl_tag_handler(void *user_data, flickr_tag* tag)
{
  fprintf(stderr, "%s: photo %s tag: id %s author %s raw '%s' cooked '%s'\n",
          program, tag->photo->id,
          tag->id, tag->author, tag->raw, tag->cooked);
}
#endif

static int
command_photos_getInfo(flickcurl* fc, int argc, char *argv[])
{
  flickr_photo* photo;

#if 0
  flickcurl_set_tag_handler(fc, command_flickcurl_tag_handler, NULL);
#endif

  photo=flickcurl_photos_getInfo(fc, argv[1]);

  if(photo) {
    int i;
    
    fprintf(stderr, "Found photo with URI %s ID %s and %d tags\n",
            photo->uri, photo->id, photo->tags_count);

    for(i=0; i < photo->tags_count; i++) {
      flickr_tag* tag=photo->tags[i];
      fprintf(stderr, "tag %d): id %s author %s raw '%s' cooked '%s'\n",
              i, tag->id, tag->author, tag->raw, tag->cooked);
    }
    free_flickr_photo(photo);
  }
  
  return (photo != NULL);
}


static struct {
  const char*     name;
  const char*     description;
  command_handler handler;
  int             min;
  int             max;
} commands[] = {
  /* name, min, handler */
  {"test-echo", "Test echo - KEY VALUE",
   command_test_echo,  2, 2},
  {"photos-getInfo", "Get information about a photo - PHOTO-ID", 
   command_photos_getInfo,  1, 1},

  {NULL, NULL, NULL, 0, 0}
};  
  


static const char *title_format_string="Flickr API utility %s\n";


int
main(int argc, char *argv[]) 
{
  flickcurl *fc;
  int rc=0;
  const char* config_file="flickr.conf";
  int usage=0;
  int help=0;
  int cmd_index= -1;
  int read_auth=1;
  int i;
  
  program=my_basename(argv[0]);

  while (!usage && !help)
  {
    int c;
    char* auth_token=NULL;
    
#ifdef HAVE_GETOPT_LONG
    int option_index = 0;

    c = getopt_long (argc, argv, GETOPT_STRING, long_options, &option_index);
#else
    c = getopt (argc, argv, GETOPT_STRING);
#endif
    if (c == -1)
      break;

    switch (c) {
      case 0:
      case '?': /* getopt() - unknown option */
        usage=1;
        break;

      case 'a':
        /* Exchange the mini-token for a full token */
        auth_token=flickcurl_auth_getFullToken(fc, optarg);
        if(!auth_token) {
          fprintf(stderr, 
                  "%s: Could not find auth_token in getFullToken response\n",
                  program);
        } else {
          FILE* fh;

          flickcurl_set_auth_token(fc, auth_token);
          
          fh=fopen(config_file, "w");
          fputs("[flickr]\n", fh);
          fprintf(fh,
                  "[flickr]\napi_key=%s\nsecret=%s\nauth_token=%s\n", 
                  flickcurl_get_api_key(fc),
                  flickcurl_get_shared_secret(fc),
                  flickcurl_get_auth_token(fc));
          fclose(fh);
          read_auth=0;
        }
        
        break;

      case 'h':
        help=1;
        break;

      case 'v':
        fputs(flickcurl_version_string, stdout);
        fputc('\n', stdout);

        exit(0);
    }
    
  }

  argv+=optind;
  argc-=optind;
  
  if(!help && argc < 1)
    usage=2; /* Title and usage */

  if(!help && !argc) {
    fprintf(stderr, "%s: No command given\n", program);
    usage=1;
    goto usage;
  }

  if(usage || help)
    goto usage;


  /* Initialise the Flickr library */
  fc=flickcurl_new();
  if(!fc) {
    rc=1;
    goto tidy;
  }

  flickcurl_set_error_handler(fc, my_message_handler, NULL);

  if(read_auth && !access((const char*)config_file, R_OK)) {
    if(read_ini_config(config_file, "flickr", fc, my_set_config_var_handler)) {
      rc=1;
      goto tidy;
    }
  }
  
  for(i=0; commands[i].name; i++)
    if(!strcmp(argv[0], commands[i].name)) {
      cmd_index=i;
      break;
    }
  if(cmd_index < 0) {
    fprintf(stderr, "%s: No such command `%s'\n", program, argv[0]);
    usage=1;
    goto usage;
  }


 usage:
  if(usage) {
    if(usage>1) {
      fprintf(stderr, title_format_string, flickcurl_version_string);
      fputs("Flickcurl home page: ", stderr);
      fputs(flickcurl_home_url_string, stderr);
      fputc('\n', stderr);
      fputs(flickcurl_copyright_string, stderr);
      fputs("\nLicense: ", stderr);
      fputs(flickcurl_license_string, stderr);
      fputs("\n\n", stderr);
    }
    fprintf(stderr, "Try `%s " HELP_ARG(h, help) "' for more information.\n",
            program);
    exit(1);
  }

  if(help) {
    printf(title_format_string, flickcurl_version_string);
    puts("Call the Flickr API to get information.");
    printf("Usage: %s [OPTIONS] command args...\n\n", program);

    fputs(flickcurl_copyright_string, stdout);
    fputs("\nLicense: ", stdout);
    puts(flickcurl_license_string);
    fputs("Flickcurl home page: ", stdout);
    puts(flickcurl_home_url_string);

    fputs("\n", stdout);

    puts(HELP_TEXT("h", "help            ", "Print this help, then exit"));
    puts(HELP_TEXT("v", "version         ", "Print the flickcurl version"));

    fputs("\nCommands\n", stdout);
    for(i=0; commands[i].name; i++)
      printf("  %-20s  %s\n", commands[i].name, commands[i].description);

    exit(0);
  }


  /* Perform the Flickr API call */
  rc=commands[cmd_index].handler(fc, argc, argv);

 tidy:
  if(fc)
    flickcurl_free(fc);

  return(rc);
}