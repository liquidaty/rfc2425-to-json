/* ----------------------------------------------------------------------
   This file is part of rfc2425-to-json, a program and library for
   parsing text input conforming to rfc2425 and related specification(s)

   Copyright (C) 2024 Liquidaty (info@liquidaty.com)

   This program is released under the MIT license; see
   https://github.com/liquidaty/rfc2425-to-json/blob/master/LICENSE
   ----------------------------------------------------------------------
*/

#ifndef RFC2425PARSER_DATA_H
#define RFC2425PARSER_DATA_H

struct rfc2425parser_data;
typedef struct rfc2425parser_data * rfc2425parser_handle;

rfc2425parser_handle rfc2425parser_new();
void rfc2425parser_set_semicolon(rfc2425parser_handle, char);
void rfc2425parser_set_rfc5545(rfc2425parser_handle, char);
void rfc2425parser_delete(rfc2425parser_handle);

void rfc2425parser_data_print(rfc2425parser_handle h);
void rfc2425parser_data_clear(rfc2425parser_handle h);

void rfc2425parser_add_line(rfc2425parser_handle h, char *s);
void rfc2425parser_add_keyword(rfc2425parser_handle h, char *s);

void rfc2425parser_process(rfc2425parser_handle h);

#endif
