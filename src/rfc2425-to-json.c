/* ----------------------------------------------------------------------
   This file is part of rfc2425-to-json, a program and library for
   parsing text input conforming to rfc2425 and related specification(s)

   Copyright (C) 2024 Liquidaty (info@liquidaty.com)

   This program is released under the MIT license; see
   https://github.com/liquidaty/rfc2425-to-json/blob/master/LICENSE
   ----------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <jsonwriter.h>
#include <rfc2425parser.h>

#include <rfc2425parser.tab.h>
#include <rfc2425parser.lex.h>

struct rfc2425parser_line {
  struct rfc2425parser_line *next;
  char *s;
};

struct rfc2425parser_blocks {
  struct rfc2425parser_blocks *prior;
  char *s;
};

/*
  BEGIN: Y
  BEGIN: X
  ...
  END: X: queue up an array X pop
  BEGIN: X: cancel the array X pop
  ...
  END: X: queue up an array X pop
  END: Y: pop the X array end; queue Y array pop
  BEGIN: Y: cancel. queue up an array Y pop
*/

struct rfc2425parser_data {
  struct rfc2425parser_line *lines;
  struct rfc2425parser_line **next_line;
  struct rfc2425parser_line *keyword_lines;
  char *prior_end;
  struct rfc2425parser_line **next_keyword_line;
  jsonwriter_handle jw;
  struct rfc2425parser_blocks *rfc2425parser_blocks;
  const char **list_keywords;
  unsigned int semicolon:1;
};

static int lines_cmp(struct rfc2425parser_line *l1, struct rfc2425parser_line *l2) {
  while(l1 && l2) {
    if(l1->s && l2->s) {
      int rc = strcmp(l1->s, l2->s);
      if(rc)
        return rc;
    } else if(!l2->s) // && l1->s
      return 1;
    else // !l1->s && l2->s
      return -1;
    l1 = l1->next;
    l2 = l2->next;
  }

  return (!l1 && !l2) ? 0 : l1 ? 1 : -1;
}

static void print_lines(FILE *out, struct rfc2425parser_line *lines) {
  for(struct rfc2425parser_line *line = lines; line; line = line->next)
    fprintf(out, "%s", line->s);
  fprintf(out, "\n\n");
}

static void rfc2425parser_free_lines(struct rfc2425parser_line *lines) {
  for(struct rfc2425parser_line *next, *line = lines; line; line = next) {
    next = line->next;
    free(line->s);
    free(line);
  }
}

static void rfc2425parser_clear_lines(struct rfc2425parser_line **lines) {
  rfc2425parser_free_lines(*lines);
  *lines = NULL;
}

static char *lines_to_str(struct rfc2425parser_line **lines) {
  char *s = NULL;
  size_t len = 0;
  for(struct rfc2425parser_line *line = *lines; line; line = line->next)
    len += line->s ? strlen(line->s) : 0;

  if(len) {
    s = malloc(len + 1);
    if(s) {
      len = 0;
      for(struct rfc2425parser_line *line = *lines; line; line = line->next) {
        if(line->s) {
          memcpy(s + len, line->s, strlen(line->s));
          len += strlen(line->s);
        }
      }
      s[len] = '\0';
    }
  }
  rfc2425parser_clear_lines(lines);
  return s;
}

void rfc2425parser_add_line(rfc2425parser_handle h, char *s) {
  struct rfc2425parser_line *line = calloc(1, sizeof(*line));
  line->s = s;
  if(!h->lines)
    h->lines = line;
  else
    *h->next_line = line;
  h->next_line = &line->next;
}

void rfc2425parser_add_keyword(rfc2425parser_handle h, char *s) {
  struct rfc2425parser_line *line = calloc(1, sizeof(*line));
  line->s = s;
  if(!h->keyword_lines)
    h->keyword_lines = line;
  else
    *h->next_keyword_line = line;
  h->next_keyword_line = &line->next;
}

static void rfc2425parser_block_begin(rfc2425parser_handle h, char **str_p) {
  char *s = *str_p;
  struct rfc2425parser_blocks *ks = calloc(1, sizeof(*ks));
  if(ks) {
    ks->s = s;
    *str_p = NULL;
    ks->prior = h->rfc2425parser_blocks;
    h->rfc2425parser_blocks = ks;
  }
  free(h->prior_end);
  h->prior_end = NULL;
}

static char *rfc2425parser_block_end(rfc2425parser_handle h) {
  char *s = NULL;
  if(h) {
    struct rfc2425parser_blocks *ks = h->rfc2425parser_blocks;
    if(ks) {
      s = ks->s;
      h->rfc2425parser_blocks = ks->prior;
      free(ks);
    }
  }
  return s;
}

void rfc2425parser_process_internal(rfc2425parser_handle h, const char *keyword,
                                    char **strp) {
  char *str = *strp;
  if(!strcmp(keyword, "BEGIN")) {
    // did we just begin the same type of array item we just ended?
    if(h->prior_end && !strcmp(str, h->prior_end)) {
      ; // continuation of what was just ended. do not create new array
    } else if(h->prior_end) {
      // not a continuation of what just ended, so end that array
      // and start a new obj
      jsonwriter_end_array(h->jw);
      free(h->prior_end);
      h->prior_end = NULL;
      jsonwriter_object_array(h->jw, str);
    } else {
      jsonwriter_object_array(h->jw, str);
    }
    jsonwriter_start_object(h->jw);
    rfc2425parser_block_begin(h, strp);
  } else {
    if(!strcmp(keyword, "END")) {
      char *old_s = rfc2425parser_block_end(h);
      if(str && old_s && strcmp(str, old_s)) {
        fprintf(stderr, "Warning: mismatched BEGIN/END tags:\n");
        fprintf(stderr, "BEGIN:\n  %s", old_s);
        fprintf(stderr, "\nEND:\n  %s", str);
        fprintf(stderr, "\n\n");
      }
      if(h->prior_end) { // two ends in a row: finish the prior array
        jsonwriter_end_array(h->jw);
        free(h->prior_end);
      }
      jsonwriter_end_object(h->jw);
      h->prior_end = str;
      *strp = NULL;
      free(old_s);
    } else {
      if(h->prior_end) { // end followed by neither begin nor end
        jsonwriter_end_array(h->jw);
        free(h->prior_end);
        h->prior_end = NULL;
      }
      jsonwriter_object_cstr(h->jw, keyword, str);
    }
  }
}

static int str_match_any(const char *str, const char *list[]) {
  if(str && list) {
    for(int i = 0; list[i]; i++)
      if(!strcmp(str, list[i]))
        return 1;
  }
  return 0;
}

void rfc2425parser_process(rfc2425parser_handle h) {
  if(!(h && h->jw)) return;

  char *keyword = lines_to_str(&h->keyword_lines);
  char *str = lines_to_str(&h->lines);
  char *semicolon = strchr(keyword, ';');
  int list_keyword = 0;
  if(h->semicolon &&
     (
      (semicolon && semicolon != keyword)
      || (list_keyword = str_match_any(keyword, h->list_keywords))
      )) {
    // keyword=A;B...
    // str=...
    // treat this the same as:
    // BEGIN:A
    // PARAMETERS:B
    // VALUE:str
    // END:A
    char *parameters = NULL;
    if(semicolon) {
      *semicolon = '\0';
      if(!list_keyword)
        list_keyword = str_match_any(keyword, h->list_keywords);
      parameters = strdup(semicolon + 1);
    }
    if(!list_keyword) { // e.g. "DTSTART;VALUE=DATE". Treat as multiple single-values
      rfc2425parser_process_internal(h, keyword, &str);
      if(parameters && *parameters) {
        char *keyword_PARAMETERS;
        asprintf(&keyword_PARAMETERS, "%s-PARAMETERS", keyword);
        rfc2425parser_process_internal(h, keyword_PARAMETERS, &parameters);
        free(keyword_PARAMETERS);
      }
    } else {            // e.g. "ATTENDEE:ZZZ" or "ATTENDEE;XXX:ZZZ"
      char *keyword_BEGIN = strdup(keyword);
      char *keyword_END = strdup(keyword);
      rfc2425parser_process_internal(h, "BEGIN", &keyword_BEGIN);
      if(parameters && *parameters)
        rfc2425parser_process_internal(h, "PARAMETERS", &parameters);
      rfc2425parser_process_internal(h, "VALUE", &str);
      rfc2425parser_process_internal(h, "END", &keyword_END);
      free(keyword_BEGIN);
      free(keyword_END);
    }
    free(parameters);
  } else {
    rfc2425parser_process_internal(h, keyword, &str);
  }
  rfc2425parser_clear_lines(&h->lines);
  free(keyword);
  free(str);
}

rfc2425parser_handle rfc2425parser_new() {
  rfc2425parser_handle h = calloc(1, sizeof(*h));
  if(h) {
    if((h->jw = jsonwriter_new(stdout)))
      jsonwriter_start_object(h->jw);
  }
  return h;
}

void rfc2425parser_delete(rfc2425parser_handle h) {
  if(h) {
    if(h->jw) {
      jsonwriter_end_all(h->jw);
      jsonwriter_delete(h->jw);
    }
    rfc2425parser_clear_lines(&h->lines);
    rfc2425parser_clear_lines(&h->keyword_lines);
    free(h->prior_end);
    for(struct rfc2425parser_blocks *prior, *ks = h->rfc2425parser_blocks; ks; ks = prior) {
      prior = ks->prior;
      free(ks->s);
      free(ks);
    }
    h->rfc2425parser_blocks = NULL;

    free(h);
  }
}

int rfc2425parser_parse_file(rfc2425parser_handle h, FILE *f) {
  void *scanner;
  int err = rfc2425parser_lex_init(&scanner);
  if(!err) {
    YY_BUFFER_STATE buf = rfc2425parser__create_buffer(f, 4096, scanner);
    rfc2425parser__switch_to_buffer(buf, scanner);
    rfc2425parser_parse(scanner, h);
    rfc2425parser__delete_buffer(buf, scanner);
    rfc2425parser_lex_destroy(scanner);
  }
  return err;
}

static const char *rfc5545_list_keywords[] = { "ATTENDEE", NULL };
void rfc2425parser_set_semicolon(rfc2425parser_handle h, char value) {
  h->semicolon = !!value;
}

void rfc2425parser_set_rfc5545(rfc2425parser_handle h, char value) {
  h->semicolon = !!value;
  h->list_keywords = rfc5545_list_keywords;
}

#ifdef RFC2425_BUILD_MAIN
static int usage(FILE *out) {
  const char *usage_s[] =
    {
      "Usage: rfc2425parse <filename> [--semicolon] [--rfc5545,--ics]",
      "  e.g. rfc2425parse myfile.ics",
      "",
      "Converts RFC2425-conforming text to JSON. Options:",
      "  --semicolon: convert semicolon-containing keywords to a pair of properties as",
      "               follows:",
      "               - K:V, where K = prefix before semicolon and V = value after",
      "                 the colon",
      "               - K-PARAMETERS:V, where V = value between semicolon and colon",
      "  --rfc5545:   same as --semicolon, except that ATTENDEE is always treated as a",
      "               list (as if offset by BEGIN:ATTENDEE and END:ATTENDEE)",
      "  --ics:       alias for --rfc5545",
      "",
      "BEGIN/END blocks, and specific keywords (e.g. ATTENDEE) when used with",
      "the --ics/--rfc5545 option, are treated as lists of objects; if the"
      "closing END (or next line in the case of semicolon-containing keyword)",
      "is immediately followed by the same keyword, it is treated as the next",
      "element in the list",
      "",
      "See also:",
      "  https://datatracker.ietf.org/doc/html/rfc2425",
      "  https://datatracker.ietf.org/doc/html/rfc5545",
      "",
      NULL
    };

  for(int i = 0; usage_s[i]; i++)
    fprintf(out, "%s\n", usage_s[i]);
  return (out == stdout ? 0 : 1);
}

int main(int argc, const char *argv[]) {
  char rfc5545 = 0;
  char semicolon = 0;
  if(argc < 2)
    return usage(stderr);

  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
      return usage(stdout);
  }

  int err = 0;
  if(!err) {
    const char *fname = argv[1];
    FILE *f = !strcmp(fname, "-") ? stdin : fopen(fname, "rb");
    if(!f) {
      perror(fname);
      err = 1;
    } else {
      for(int i = 2; i < argc; i++) {
        const char *arg = argv[i];
        if(!strcmp(arg, "--semicolon"))
          semicolon = 1;
        else if(!strcmp(arg, "--rfc5545") || !strcmp(arg, "--ics"))
          rfc5545 = 1;
        else {
          fprintf(stderr, "Unrecognized option: %s\n", arg);
          err = 1;
        }
      }
    }
    if(!err) {
      rfc2425parser_handle h = rfc2425parser_new();
      if(semicolon)
        rfc2425parser_set_semicolon(h, 1);
      if(rfc5545)
        rfc2425parser_set_rfc5545(h, 1);
      if(h) {
        rfc2425parser_parse_file(h, f);
        rfc2425parser_delete(h);
      }
      if(f != stdin)
        fclose(f);
    }
  }
  return err;
}
#endif
