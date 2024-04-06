 /* ----------------------------------------------------------------------
   This file is part of rfc2425-to-json, a program and library for
   parsing text input conforming to rfc2425 and related specification(s)

   Copyright (C) 2024 Liquidaty (info@liquidaty.com)

   This program is released under the MIT license; see
   https://github.com/liquidaty/rfc2425-to-json/blob/master/LICENSE
   ----------------------------------------------------------------------
 */
%option header-file="rfc2425parser.lex.h"
%option reentrant bison-bridge
%option prefix="rfc2425parser_"
%option noyywrap nounput noinput

%{
#include <rfc2425parser.tab.h>

static char *strdup_and_escape(const char *s, unsigned truncate_len) {
  size_t len = strlen(s) - truncate_len;
  char *new_s = malloc(len + 1);
  if(new_s) {
    size_t j = 0;
    for(size_t i = 0; i < len; i++) {
      char c = *(s + i);
      switch(c) {
      case '\\':
        if(i + 1 < len) {
          i++;
          switch(*(s + i)) {
          case 'n':
          case'N':
            new_s[j++] = '\n';
            break;
          case '\\':
            new_s[j++] = '\\';
            break;
          case ',':
            new_s[j++] = ',';
            break;
          default:
            new_s[j++] = '\\';
            new_s[j++] = *(s + i);
          }
        }
        break;
      default:
        new_s[j++] = c;
      }
    }
    new_s[j] = '\0';
  }
  return new_s;
}
%}

%x VALUEPARSE

%%

[^:\n]*:                { yylval->str = strdup_and_escape(yytext, 1); BEGIN VALUEPARSE; return KEYWORD; }
[^:\n]+[\r][\n][ \t]    { yylval->str = strdup_and_escape(yytext, 3); return KEYWORD; }

<VALUEPARSE>.*[\r][\n][ \t] { yylval->str = strdup_and_escape(yytext, 3); return LINE; }
<VALUEPARSE>.*[\r]?[\n]     { yylval->str = strdup_and_escape(yytext, 2); BEGIN INITIAL; return LINE; }
.                       { yylval->str = NULL; return OTHER_CHAR; }
<<EOF>>                 { yylval->str = NULL; return INPUT_END; }

%%
