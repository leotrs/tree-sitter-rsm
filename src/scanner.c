#include <tree_sitter/parser.h>
#include <stdio.h>
#include <wctype.h>

enum TokenType {
  UPTO_BRACE_OR_COMMA_TEXT = 0,
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


static bool looking_for(const bool *valid_symbols, enum TokenType type) {
  return valid_symbols[type];
}

static bool looking_for_everything(const bool *valid_symbols) {
  for (int i = 0; i<NUMBER_OF_TOKEN_TYPES; i++) {
    if (!valid_symbols[i]) {
      return false;
    }
  }
  return true;
}

static bool looking_for_paragraph_end_only(const bool *valid_symbols) {
  for (int i = 0; i<NUMBER_OF_TOKEN_TYPES; i++) {
    if (i == PARAGRAPH_END) {
      continue;
    }
    if (valid_symbols[i]) {
      return false;
    }
  }
  return valid_symbols[PARAGRAPH_END];
}

static bool looking_for_paragraph_end_and_other(const bool *valid_symbols) {
  if (!valid_symbols[PARAGRAPH_END]) {
    return false;
  }
  for (int i = 0; i<NUMBER_OF_TOKEN_TYPES; i++) {
    if (i == PARAGRAPH_END) {
      continue;
    }
    if (valid_symbols[i]) {
      return true;
    }
  }
  return false;
}

static void show_beginning_debug_message(const bool *valid_symbols) {
  if (!getenv("TREE_SITTER_DEBUG")) { return; }

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

static void show_lookahead(TSLexer *lexer) {
  if (!getenv("TREE_SITTER_DEBUG")) { return; }
  if (32 <= lexer->lookahead && lexer->lookahead <= 127) {
    printf("--> lookahead: '%c'\n", lexer->lookahead);
  } else {
    printf("--> lookahead: %d\n", lexer->lookahead);
  }
}

void *tree_sitter_rsm_external_scanner_create() {
  return NULL;
}

void tree_sitter_rsm_external_scanner_destroy(void *p) {
}

unsigned tree_sitter_rsm_external_scanner_serialize(void *p, char *buffer) {
  return 0;
}

void tree_sitter_rsm_external_scanner_deserialize(void *p, const char *buffer, unsigned n) {
}

static void debug_log(const char *msg) {
  if (!getenv("TREE_SITTER_DEBUG")) { return; }
  printf("--> %s\n", msg);
}

static bool success(TSLexer *lexer, enum TokenType type) {
  lexer->result_symbol = type;
  debug_log("SUCCESS");
  return true;
}

static bool failure(TSLexer *lexer) {
  debug_log("FAILURE");
  return false;
}

static void skip_whitespace(TSLexer *lexer) {
  while (iswspace(lexer->lookahead)) {
    lexer->advance(lexer, true);
  }
}


static void skip_carriage_return(TSLexer *lexer) {
  if (lexer->lookahead == '\r') {
    lexer->advance(lexer, true);
  }
}

static bool scan_paragraph_end(void *payload, TSLexer *lexer) {
  debug_log("trying PARAGRAPH_END");
  // A paragraph may end in a blank line ("\n\n"), or in the Halmos of the enclosing
  // block ("::").  In the latter case, make sure to use mark_end() to not consume the
  // Halmos, as it will be consumed elsewhere.

  skip_carriage_return(lexer);
  if (lexer->lookahead == '\n') {
    lexer->advance(lexer, true);
    skip_carriage_return(lexer);
    if (lexer->lookahead == '\n') {
      return success(lexer, PARAGRAPH_END);
    } else {
      skip_whitespace(lexer);
      if (lexer->lookahead == ':') {
	lexer->mark_end(lexer);
	lexer->advance(lexer, false);
	if (lexer->lookahead == ':') {
	  return success(lexer, PARAGRAPH_END);
	}
      }
      return failure(lexer);
    }
  }
  else if (lexer->lookahead == ':') {
    lexer->mark_end(lexer);
    lexer->advance(lexer, false);
    if (lexer->lookahead == ':') {
      return success(lexer, PARAGRAPH_END);
    } else {
      return failure(lexer);
    }
  }
  return failure(lexer);
}

static bool scan_arbitrary_text(void *payload, TSLexer *lexer) {
  // DO NOT call skip_whitespace as we want to consume, not skip the whitespace
  while (lexer->lookahead == '\n' || lexer->lookahead == '\r') {
    lexer->advance(lexer, true);
  }

  int count = 0;
  bool escape_next = false;
  while (
	 escape_next ||
	 (
	  lexer->lookahead != ':'     // delimiter
	  && lexer->lookahead != '\n' // newline
	  && lexer->lookahead != '\r' // windows
	  && lexer->lookahead != '{'  // inline meta
	  && lexer->lookahead != '}'  // inline meta
	  && lexer->lookahead != '$'  // math region
	  && lexer->lookahead != '`'  // code region
	  && lexer->lookahead != '*'  // strong region
	  && lexer->lookahead != '/'  // emphas region
	  && lexer->lookahead != '#'  // section header
	  && lexer->lookahead != '%'  // comment
	  && lexer->lookahead != '\0' // EOF
	  )
	 ) {
    if (!iswspace(lexer->lookahead)) count++;
    escape_next = lexer->lookahead == '\\';
    lexer->advance(lexer, false);
  }

  if (count > 0) {
    return success(lexer, TEXT);
  } else {
    return failure(lexer);
  }
}

static bool scan_asis_text(void *payload, TSLexer *lexer, const char terminal) {
  debug_log("trying ASIS_TEXT\n");
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

static bool scan_asis_dollar_text(void *payload, TSLexer *lexer) {
  if (scan_asis_text(payload, lexer, '$')) {
    return success(lexer, ASIS_DOLLAR_TEXT);
  } else {
    return failure(lexer);
  }
}

static bool scan_asis_backtick_text(void *payload, TSLexer *lexer) {
  if (scan_asis_text(payload, lexer, '`')) {
    return success(lexer, ASIS_BACKTICK_TEXT);
  } else {
    return failure(lexer);
  }
}

static bool scan_asis_halmos_text(void *payload, TSLexer *lexer) {
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
	return success(lexer, ASIS_HALMOS_TEXT);
      }
    }
    count++;
    lexer->advance(lexer, false);
  }
  return failure(lexer);
}

static bool scan_asis_two_dollars_text(void *payload, TSLexer *lexer) {
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
	return success(lexer, ASIS_TWO_DOLLARS_TEXT);
      }
    }
    count++;
    lexer->advance(lexer, false);
  }
  return failure(lexer);
}

static bool scan_asis_three_backticks_text(void *payload, TSLexer *lexer) {
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
	  return success(lexer, ASIS_THREE_BACKTICKS_TEXT);
	}
      }
    }
    count++;
    lexer->advance(lexer, false);
  }
  return failure(lexer);
}

bool tree_sitter_rsm_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  show_beginning_debug_message(valid_symbols);
  show_lookahead(lexer);

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

  if (valid_symbols[UPTO_BRACE_OR_COMMA_TEXT]) {
    skip_whitespace(lexer);
    debug_log("trying UPTO_BRACE_OR_COMMA_TEXT");
    while (
	   lexer->lookahead != ':'
	   && lexer->lookahead != '\n'
	   && lexer->lookahead != '{'
	   && lexer->lookahead != '}'
	   && lexer->lookahead != ','
	   && lexer->lookahead != '\0'
	   ) {
      lexer->advance(lexer, false);
    }
    if (lexer->lookahead == ',' || lexer->lookahead == '}') {
      return success(lexer, UPTO_BRACE_OR_COMMA_TEXT);
    } else {
      return failure(lexer);
    }
  }

  if (valid_symbols[TEXT]) {
    return scan_arbitrary_text(payload, lexer);
  }

  debug_log("Reached the bottom!");
  return failure(lexer);
}
