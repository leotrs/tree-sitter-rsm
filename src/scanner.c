#include <tree_sitter/parser.h>
#include <stdio.h>
#include <wctype.h>
#include <unistd.h>

enum TokenType {
  UPTO_BRACE_OR_COMMA_TEXT,
  ASIS_DOLLAR_TEXT,
  ASIS_TWO_DOLLARS_TEXT,
  ASIS_BACKTICK_TEXT,
  ASIS_THREE_BACKTICKS_TEXT,
  ASIS_HALMOS_TEXT,
  TEXT,
  PARAGRAPH_END,
  // Since the first one is assigned to zero and they are assigned consecutive integers,
  // the last one is a dummy value that is always equal to the number of values that
  // this Enum contains.  This is then used to iterate over valid_symbols, whose length
  // is thus always equal to NUMBER_OF_TOKEN_TYPES.
  NUMBER_OF_TOKEN_TYPES,
};


bool looking_for(const bool *valid_symbols, enum TokenType type) {
  return valid_symbols[type];
}

bool looking_for_everything(const bool *valid_symbols) {
  bool result = true;
  for (int i; i<NUMBER_OF_TOKEN_TYPES; i++) {
    if (!valid_symbols[i]) {
      result = false;
    }
  }
  return result;
}

bool looking_for_paragraph_end_only(const bool *valid_symbols) {
  bool result = true;
  for (int i; i<NUMBER_OF_TOKEN_TYPES; i++) {
    if (i == PARAGRAPH_END) {
      result = result && valid_symbols[i];
    } else {
    result = result && !valid_symbols[i];
    }
  }
  return result;
}

bool looking_for_paragraph_end_and_other(const bool *valid_symbols) {
  bool result = false;
  for (int i; i<NUMBER_OF_TOKEN_TYPES; i++) {
    if (i == PARAGRAPH_END) {
      continue;
    } else {
      if (valid_symbols[i]) {
	result = true;
      }
    }
  }
  return result && valid_symbols[PARAGRAPH_END];
}

struct ScannerState {
    uint32_t num_tokens_found_without_consuming_chars;
};

void *tree_sitter_RSM_external_scanner_create() {
    return calloc(0, sizeof(struct ScannerState));
}

void tree_sitter_RSM_external_scanner_destroy(void *p) {
    free(p);
}

unsigned tree_sitter_RSM_external_scanner_serialize(void *p, char *buffer) {
  struct ScannerState *state = (struct ScannerState *)p;
  uint32_t count = state->num_tokens_found_without_consuming_chars;
  buffer[0] = (count >> 24) & 0xff;
  buffer[1] = (count >> 16) & 0xff;
  buffer[2] = (count >> 8) & 0xff;
  buffer[3] = (count) & 0xff;
  return 4;
}

void tree_sitter_RSM_external_scanner_deserialize(void *p, const char *buffer, unsigned n) {
  if (n < 4) {return;}
  uint32_t count = (
		    (((uint32_t) buffer[0]) << 24) |
		    (((uint32_t) buffer[1]) << 16) |
		    (((uint32_t) buffer[2]) << 8) |
		    (((uint32_t) buffer[3]))
		    );
  struct ScannerState *state = (struct ScannerState *)p;
  state->num_tokens_found_without_consuming_chars = count;
}

bool scan_paragraph_end(void *payload, TSLexer *lexer) {
  printf("--> trying PARAGRAPH_END\n");
  // A paragraph may end in a blank line ("\n\n") or in the Halmos of the enclosing
  // block "::".  In the latter case, make sure to use makr_end() so we do not consume
  // the Halmos (it will be consumed by the enclosing block).
  if (lexer->lookahead == '\n') {
    lexer->advance(lexer, false);
    if (lexer->lookahead == '\n') {
	lexer->result_symbol = PARAGRAPH_END;
	printf("--> SUCCESS\n");
	return true;
    } else {
      printf("--> FAILURE\n");
      return false;
    }
  }
  else if (lexer->lookahead == ':') {
    lexer->mark_end(lexer);
    lexer->advance(lexer, false);
    if (lexer->lookahead == ':') {
      lexer->result_symbol = PARAGRAPH_END;
      struct ScannerState *state = (struct ScannerState *)payload;
      state->num_tokens_found_without_consuming_chars++;
      printf("--> SUCCESS\n");
      return true;
    } else {
      printf("--> FAILURE\n");
      return false;
    }
  }
  printf("--> FAILURE\n");
  return false;
}

void skip_whitespace(TSLexer *lexer) {
  while (iswspace(lexer->lookahead)) {
    lexer->advance(lexer, true);
  }
}

bool scan_arbitrary_text(void *payload, TSLexer *lexer) {
    printf("--> trying TEXT\n");
    skip_whitespace(lexer);

    int count = 0;
    while (lexer->lookahead != ':'     // cannot start at delimiter
	   && lexer->lookahead != '\n' // cannot start at newline
	   && lexer->lookahead != '$'  // math region
	   && lexer->lookahead != '`'  // code region
	   && lexer->lookahead != '*'  // strong region
	   && lexer->lookahead != '/'  // emphas region
	   && lexer->lookahead != '\0' // EOF
	   && lexer->lookahead != '#'  // section header
	   ) {
      count++;
      lexer->advance(lexer, false);
    }
    if (count > 0) {
      lexer->result_symbol = TEXT;
      printf("--> SUCCESS\n");
      return true;
    } else {
      printf("--> FAILURE\n");
      return false;
    }
}

bool scan_asis_text(void *payload, TSLexer *lexer, const char terminal) {
  printf("--> trying ASIS_TEXT\n");
  skip_whitespace(lexer);
  // ASIS_TEXT usually occurs in the content of special tags such as :math:, :code: or
  // their block forms :mathblock: and :codeblock:.  It cannot start with an open brace
  // or a colon because that means there is a meta region to be parsed before the
  // content actually starts.
  if (lexer->lookahead == '{' || lexer->lookahead == ':') {
    return false;
  }
  int count = 0;
  while (lexer->lookahead != '\0') {
    if (lexer->lookahead == terminal) {
      // This function DOES NOT set lexer->result_symbol, that must be handled by the caller
      return (count > 0);
    }
    count++;
    lexer->advance(lexer, false);
  }
  return false;
}

bool scan_asis_dollar_text(void *payload, TSLexer *lexer) {
  if (scan_asis_text(payload, lexer, '$')) {
    lexer->result_symbol = ASIS_DOLLAR_TEXT;
    printf("--> SUCCESS\n");
    return true;
  } else {
    printf("--> FAILURE\n");
    return false;
  }
}

bool scan_asis_backtick_text(void *payload, TSLexer *lexer) {
  if (scan_asis_text(payload, lexer, '`')) {
    lexer->result_symbol = ASIS_BACKTICK_TEXT;
    printf("--> SUCCESS\n");
    return true;
  } else {
    printf("--> FAILURE\n");
    return false;
  }
}

bool scan_asis_halmos_text(void *payload, TSLexer *lexer) {
  skip_whitespace(lexer);
  // ASIS_TEXT usually occurs in the content of special tags such as :math:, :code: or
  // their block forms :mathblock: and :codeblock:.  It cannot start with an open brace
  // or a colon because that means there is a meta region to be parsed before the
  // content actually starts.
  if (lexer->lookahead == '{' || lexer->lookahead == ':') {
    return false;
  }
  int count = 0;
  // Remember to NOT consume the Halmos, as the grammar is expecting to see it
  while (lexer->lookahead != '\0') {
    if (lexer->lookahead == ':') {
      lexer->mark_end(lexer);
      count++;
      lexer->advance(lexer, false);
      if (lexer->lookahead == ':') {
	count++;
	lexer->advance(lexer, false);
	printf("--> SUCCESS\n");
	lexer->result_symbol = ASIS_HALMOS_TEXT;
	return true;
      }
    }
    count++;
    lexer->advance(lexer, false);
  }
  printf("--> FAILURE\n");
  return false;
}

bool scan_asis_two_dollars_text(void *payload, TSLexer *lexer) {
  skip_whitespace(lexer);
  // ASIS_TEXT usually occurs in the content of special tags such as :math:, :code: or
  // their block forms :mathblock: and :codeblock:.  It cannot start with an open brace
  // or a colon because that means there is a meta region to be parsed before the
  // content actually starts.
  if (lexer->lookahead == '{' || lexer->lookahead == ':') {
    return false;
  }
  int count = 0;
  // Remember to NOT consume the ending $$, as the grammar is expecting to see it
  while (lexer->lookahead != '\0') {
    if (lexer->lookahead == '$') {
      lexer->mark_end(lexer);
      count++;
      lexer->advance(lexer, false);
      if (lexer->lookahead == '$') {
	count++;
	lexer->advance(lexer, false);
	printf("--> SUCCESS\n");
	lexer->result_symbol = ASIS_TWO_DOLLARS_TEXT;
	return true;
      }
    }
    count++;
    lexer->advance(lexer, false);
  }
  printf("--> FAILURE\n");
  return false;
}

bool scan_asis_three_backticks_text(void *payload, TSLexer *lexer) {
  skip_whitespace(lexer);
  // ASIS_TEXT usually occurs in the content of special tags such as :math:, :code: or
  // their block forms :mathblock: and :codeblock:.  It cannot start with an open brace
  // or a colon because that means there is a meta region to be parsed before the
  // content actually starts.
  if (lexer->lookahead == '{' || lexer->lookahead == ':') {
    return false;
  }
  int count = 0;
  // Remember to NOT consume the ending $$, as the grammar is expecting to see it
  while (lexer->lookahead != '\0') {
    if (lexer->lookahead == '`') {
      lexer->mark_end(lexer);
      count++;
      lexer->advance(lexer, false);
      if (lexer->lookahead == '`') {
	count++;
	lexer->advance(lexer, false);
	if (lexer->lookahead == '`') {
	  printf("--> SUCCESS\n");
	  lexer->result_symbol = ASIS_THREE_BACKTICKS_TEXT;
	  return true;
	}
      }
    }
    count++;
    lexer->advance(lexer, false);
  }
  printf("--> FAILURE\n");
  return false;
}

void show_beginning_debug_message(const bool *valid_symbols) {
  printf("--> external scanner looking for: ");
  if (valid_symbols[UPTO_BRACE_OR_COMMA_TEXT]) {
    printf("UPTO_BRACE_OR_COMMA_TEXT ");
  }
  if (valid_symbols[ASIS_DOLLAR_TEXT]) {
    printf("ASIS_DOLLAR_TEXT ");
  }
  if (valid_symbols[ASIS_TWO_DOLLARS_TEXT]) {
    printf("ASIS_TWO_DOLLARS_TEXT ");
  }
  if (valid_symbols[ASIS_BACKTICK_TEXT]) {
    printf("ASIS_BACKTICK_TEXT ");
  }
  if (valid_symbols[ASIS_THREE_BACKTICKS_TEXT]) {
    printf("ASIS_THREE_BACKTICKS_TEXT");
  }
  if (valid_symbols[ASIS_HALMOS_TEXT]) {
    printf("ASIS_HALMOS_TEXT ");
  }
  if (valid_symbols[TEXT]) {
    printf("TEXT ");
  }
  if (valid_symbols[PARAGRAPH_END]) {
    printf("PARAGRAPH_END ");
  }
  printf("\n");
}

void show_lookahead(TSLexer *lexer) {
  if (32 <= lexer->lookahead && lexer->lookahead <= 127) {
    printf("--> lookahead: '%c'\n", lexer->lookahead);
  } else {
    printf("--> lookahead: %d\n", lexer->lookahead);
  }
}

bool tree_sitter_RSM_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  show_beginning_debug_message(valid_symbols);
  show_lookahead(lexer);
  /* sleep(1); */

  if (looking_for_everything(valid_symbols)) {
    // Sometimes the parser freaks out and wants to see if there is *any* token at the
    // current point.  In this case, just look for arbitrary TEXT.
    return scan_arbitrary_text(payload, lexer);
  }

  if (valid_symbols[ASIS_DOLLAR_TEXT]) {
    return scan_asis_dollar_text(payload, lexer);
  }
  if (valid_symbols[ASIS_TWO_DOLLARS_TEXT]) {
    return scan_asis_two_dollars_text(payload, lexer);
  }
  if (valid_symbols[ASIS_BACKTICK_TEXT]) {
    return scan_asis_backtick_text(payload, lexer);
  }
  if (valid_symbols[ASIS_THREE_BACKTICKS_TEXT]) {
    return scan_asis_three_backticks_text(payload, lexer);
  }
  if (valid_symbols[ASIS_HALMOS_TEXT]) {
    return scan_asis_halmos_text(payload, lexer);
  }

  if (looking_for_paragraph_end_only(valid_symbols)) {
    return scan_paragraph_end(payload, lexer);
  }

  if (looking_for_paragraph_end_and_other(valid_symbols)) {
    if (lexer->lookahead == ':') {
      return scan_paragraph_end(payload, lexer);
    } else {
      return scan_paragraph_end(payload, lexer) || scan_arbitrary_text(payload, lexer);
    }
  }

  skip_whitespace(lexer);

  if (valid_symbols[TEXT]) {
    scan_arbitrary_text(payload, lexer);
  } else if (valid_symbols[UPTO_BRACE_OR_COMMA_TEXT]) {
    printf("--> trying UPTO_BRACE_OR_COMMA_TEXT\n");
    while (lexer->lookahead != ':' && lexer->lookahead != '\n' && lexer->lookahead != '}' && lexer->lookahead != ',' && lexer->lookahead != '\0') {
      lexer->advance(lexer, false);
    }
    if (lexer->lookahead == ',' || lexer->lookahead == '}') {
      lexer->result_symbol = UPTO_BRACE_OR_COMMA_TEXT;
      printf("--> SUCCESS\n");
      return true;
    } else {
      printf("--> FAILURE\n");
      return false;
    }
  }

  return false;
}
