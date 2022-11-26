module.exports = grammar({
    name: 'RSM',

    externals: $ => [
	$.upto_brace_or_comma_text,
	$.asis_dollar_text,
	$.asis_two_dollars_text,
	$.asis_backtick_text,
	$.asis_three_backticks_text,
	$.asis_halmos_text,
	$.text,
	$.paragraph_end,
    ],

    rules: {

	////////////////////////////////////////////////////////////////////////
	// Main building blocks: manuscript, block, paragraph, inline
	////////////////////////////////////////////////////////////////////////
	source_file: $ => seq(
	    field('tag', alias(':manuscript:', $.manuscript)),
	    field('meta', optional($.blockmeta)),
	    repeat(choice($.specialblock, $.block, $.paragraph)),
	    '::'),

	block: $ => prec(1, seq(
	    field('tag', $.blocktag),
	    field('meta', optional($.blockmeta)),
	    repeat(choice(
		prec(2, $.specialblock),
		prec(1, $.block),
		prec(0, $.paragraph))),
	    '::')),

	paragraph: $ => choice(
	    $.item,
	    $.caption,
	    seq(
		optional(seq(
		    field('tag', ':paragraph:'),
		    field('meta', $.inlinemeta))),
		repeat1(choice($.specialinline, $.inline, $.text)),
		alias($.paragraph_end, 'paragraph_end')
	    )),

	inline: $ => prec(0, seq(
	    field('tag', $.inlinetag),
	    field('meta', optional($.inlinemeta)),
	    repeat(choice(
		prec(2, $.specialinline),
                prec(1, $.inline),
                prec(0, $.text))),
	    '::')),

	////////////////////////////////////////////////////////////////////////
	// Special blocks, paragraphs, and inlines
	////////////////////////////////////////////////////////////////////////
	// Special blocks/inlines look just like normal blocks/inlines except that they
	// have some special parsing rule.  For example, $foo$ has the same meaning as
	// :math:foo::.

	specialblock: $ => choice(
	    $.table,

	    // Sections have a special opening using hashtags, but their content is
	    // parsed just like any other block's.
	    seq(
		field('tag', alias(/# /, $.section)),
		field('title', $.text),
		field('meta', optional($.blockmeta)),
		repeat(choice(
		    prec(2, $.specialblock),
		    prec(1, $.block),
		    prec(0, $.paragraph))),
		'::'),
	    seq(
		field('tag', alias(/## /, $.subsection)),
		field('title', $.text),
		field('meta', optional($.blockmeta)),
		repeat(choice(
		    prec(2, $.specialblock),
		    prec(1, $.block),
		    prec(0, $.paragraph))),
		'::'),
	    seq(
		field('tag', alias(/### /, $.subsubsection)),
		field('title', $.text),
		field('meta', optional($.blockmeta)),
		repeat(choice(
		    prec(2, $.specialblock),
		    prec(1, $.block),
		    prec(0, $.paragraph))),
		'::'),

	    // Math and code blocks have special open and close delimiters ($$) and
	    // (```) respesctively AND their content is taken as-is, i.e. they do not
	    // support recursive parsing.
	    seq(field('tag', alias(token(/\$\$/), $.mathblock)),
		field('meta', optional($.blockmeta)),
		alias($.asis_two_dollars_text, $.asis_text),
		/\$\$/),
	    seq(field('tag', alias(token(/```/), $.codeblock)),
		field('meta', optional($.blockmeta)),
		alias($.asis_three_backticks_text, $.asis_text),
		/```/),
	    seq(field('tag', alias(token(':mathblock:'), $.mathblock)),
		field('meta', optional($.blockmeta)),
		alias($.asis_halmos_text, $.asis_text),
		'::'),
	    seq(field('tag', alias(token(':codeblock:'), $.codeblock)),
		field('meta', optional($.blockmeta)),
		alias($.asis_halmos_text, $.asis_text),
		'::'),

	    // Algorithms have standard open and close delimiters and have as-is
	    // (i.e. not recursive) content.
	    seq(field('tag', alias(token(":algorithm:"), $.algorithm)),
		field('meta', optional($.blockmeta)),
		alias($.asis_halmos_text, $.asis_text),
		'::')),

	// Special paragraphs are not grouped inside a single $.specialparagraph rule
	// because they can only appear in certain places, and each is referred to
	// individually as $.caption and $.item and never collectively.
	caption: $ => seq(
	    token(':caption:'),
	    optional(field('meta', $.inlinemeta)),
	    repeat1(choice($.specialinline, $.inline, $.text)),
	    alias($.paragraph_end, 'paragraph_end')),

	item: $ => seq(
	    token(':item:'),
	    optional(field('meta', $.inlinemeta)),
	    repeat1(choice($.specialinline, $.inline, $.text)),
	    alias($.paragraph_end, 'paragraph_end')),

	specialinline: $ => choice(
	    // Math and code inlines have special open and close delimimters ($) and (`)
	    // respectively AND as-is content.
	    seq(field('tag', alias(token(/\$/), $.math)),
		field('meta', optional($.inlinemeta)),
		alias($.asis_dollar_text, $.asis_text),
		/\$/),
	    seq(field('tag', alias(token(/`/), $.code)),
		field('meta', optional($.inlinemeta)),
		alias($.asis_backtick_text, $.asis_text),
		/`/),
	    seq(field('tag', alias(token(':math:'), $.math)),
		field('meta', optional($.inlinemeta)),
		alias($.asis_halmos_text, $.asis_text),
		'::'),
	    seq(field('tag', alias(token(':code:'), $.code)),
		field('meta', optional($.inlinemeta)),
		alias($.asis_halmos_text, $.asis_text),
		'::'),

	    // Special spans have special open and close delimiters and support
	    // recursive parsing.  Note *foo* is equivalent to :span:{:strong:}foo::.
	    prec.right(
		seq(field('tag', alias(token(/\*/), $.spanstrong)),
		    repeat(choice(
			prec(2, $.specialinline),
			prec(1, $.inline),
			prec(0, $.text))),
		    token('*'))),
	    prec.right(
		seq(field('tag', alias(token(/\//), $.spanemphas)),
		    repeat(choice(
			prec(2, $.specialinline),
			prec(1, $.inline),
			prec(0, $.text))),
		    token('/'))),

	    // References, citations, and URLs have standard delimiters but their
	    // content is parsed in a special way.
	    seq(field('tag', alias(token(':ref:'), $.ref)),
		field('target', alias(token(/[^,:]+/), $.text)),
		optional(seq(',', field('reftext', $.text))),
		'::'),
	    // seq(field('tag', alias(token(':url:'), $.url)),
	    // 	field('target', ???),
	    // 	optional(seq(',', field('reftext', $.text))),
	    // 	'::'),
	    // seq(field('tag', alias(token(':cite:'), $.cite)),
	    // 	field('target', ???),
	    // 	optional(seq(',', field('reftext', $.text))),
	    // 	'::')),
	),

	/////////////////////////////////////////////////////////////
	// Meta regions
	/////////////////////////////////////////////////////////////
	inlinemeta: $ => seq(
	    '{',
	    // an inline meta either contains a single pair, or a sequence of pairs with
	    // an ending comma plus a final pair without comma
	    choice(
		$.inlinemetapair,
		seq(repeat1(seq($.inlinemetapair, ',')),
		    $.inlinemetapair)),
	    '}'),

	blockmeta: $ => seq(repeat1($.blockmetapair)),

	inlinemetapair: $ => choice(
	    seq($.metatag_text, $.metavalue_text_inline),
            seq($.metatag_list, $.metavalue_list_inline),
	    $.metatag_bool),

	blockmetapair: $ => choice(
	    seq($.metatag_text, $.metavalue_text),
            seq($.metatag_list, $.metavalue_list),
	    $.metatag_bool),

	/////////////////////////////////////////////////////////////
	// Meta pair types
	/////////////////////////////////////////////////////////////
	metavalue_text: $ => (/[^\S\r\n]*[^:\n]+?\n/),

	metavalue_text_inline: $ => seq($.upto_brace_or_comma_text),

        metavalue_list: $ => choice(
	    seq('{',
                repeat1(seq($.upto_brace_or_comma_text, ',')),
                $.upto_brace_or_comma_text,
                '}'),
	    (/[^\S\r\n]*[^:{\s]+\n/)
        ),

	metavalue_list_inline: $ => seq(
            choice(
                $.upto_brace_or_comma_text,
                prec(1, seq(
                    '{',
                    repeat1(seq($.upto_brace_or_comma_text, ',')),
                    $.upto_brace_or_comma_text,
                    '}')))),

	/////////////////////////////////////////////////////////////
	// Tables
	/////////////////////////////////////////////////////////////
	table: $ => seq(
	    ':table:',
	    field('head', optional($.thead)),
	    field('body', optional($.tbody)),
	    field('caption', optional($.caption)),
	    '::'),

	thead: $ => seq(':thead:', repeat1($.tr), '::'),

	tbody: $ => seq(':tbody:', repeat1($.tr), '::'),

	tr: $ => seq(':tr:', repeat1($.td), '::'),

	td: $ => seq(
	    field('tag', token(':td:')),
	    field('meta', optional($.inlinemeta)),
	    repeat(choice(
		prec(2, $.specialinline),
                prec(1, $.inline),
                prec(0, $.text))),
	    '::'),

	/////////////////////////////////////////////////////////////
	// Tag choices
	/////////////////////////////////////////////////////////////
	inlinetag: $ => choice(
	    alias(':span:', $.span),
	    alias(':claim:', $.claim),
	    // alias(':tr:', $.tr),
	    // alias(':td:', $.td),
	),

	blocktag: $ => choice(
	    alias(':abstract:', $.abstract),
	    alias(':author:', $.author),
	    alias(':definition:', $.definition),
	    alias(':enumerate:', $.enumerate),
	    alias(':lemma:', $.lemma),
	    alias(':p:', $.subproof),
	    alias(':proof:', $.proof),
	    alias(':proposition:', $.proposition),
	    alias(':remark:', $.remark),
	    alias(':section:', $.section),
	    alias(':sketch:', $.sketch),
	    alias(':subsection:', $.subsection),
	    alias(':subsubsection:', $.subsubsection),
	    alias(':subsubsubsection:', $.subsubsubsection),
	    alias(':step:', $.step),
	    // alias(':table:', $.table),
	    // alias(':tbody:', $.tbody),
	    // alias(':thead:', $.thead),
	    alias(':theorem:', $.theorem),
	),

	metatag_text: $ => choice(
	    alias(':affiliation:', $.affiliation),
	    alias(':date:', $.date),
	    alias(':email:', $.email),
	    alias(':label:', $.label),
	    alias(':name:', $.name),
	    alias(':title:', $.title),
	),

	metatag_bool: $ => choice(
	    alias(':nonum:', $.nonum),
	    alias(':strong:', $.strong),
	    alias(':emphas:', $.emphas),
	),

        metatag_list: $ => choice(
	    alias(':keywords:', $.keywords),
	    alias(':MSC:', $.MSC),
	    alias(':types:', $.types),
	),

    }
});
