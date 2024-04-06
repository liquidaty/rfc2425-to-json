/* ----------------------------------------------------------------------
   This file is part of rfc2425-to-json, a program and library for
   parsing text input conforming to rfc2425 and related specification(s)

   Copyright (C) 2024 Liquidaty (info@liquidaty.com)

   This program is released under the MIT license; see
   https://github.com/liquidaty/rfc2425-to-json/blob/master/LICENSE
   ----------------------------------------------------------------------
*/

%define api.pure full
%lex-param {void *scanner}
%parse-param { void *scanner }
%parse-param { struct rfc2425parser_data *data }
%name-prefix "rfc2425parser_"

%code top {
  #include <rfc2425parser.h>
  #include <rfc2425parser.tab.h>
  #include <rfc2425parser.lex.h>

  void rfc2425parser_error (yyscan_t *locp, struct rfc2425parser_data *data, char const *msg);
}

%code requires
{
#include <rfc2425parser.h>
}

%union {
  char* str;
}

%token <str> KEYWORD KEYWORD_LINE LINE OTHER_CHAR INPUT_END

%destructor { free($$); } <str>

%%

%start rfc2425file ;

rfc2425file: key_line_pairs INPUT_END { (void)($2); };

key_line_pairs: key_line_pair
| key_line_pairs key_line_pair
;

key_line_pair: keywords lines { rfc2425parser_process(data); } ;

keywords: KEYWORD             { rfc2425parser_add_keyword(data, $1); }
| keywords KEYWORD            { rfc2425parser_add_keyword(data, $2); }
;

lines: LINE  { rfc2425parser_add_line(data, $1); }
| lines LINE { rfc2425parser_add_line(data, $2); }
;

%%

void rfc2425parser_error (yyscan_t *locp, struct rfc2425parser_data *data, char const *msg) {
  (void)(locp);
  (void)(data);
  (void)(msg);
}
