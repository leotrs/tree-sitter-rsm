module.exports = grammar({
    name: 'rsm',

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

    inline: $ => [
	$.blocktag,
	$.inlinetag,
	$.constructtag,
	$.blockcontent,
	$.paragraphcontent,
	$.inlinecontent,
    ],

    extras: $ => [
	$.comment,
	/\s/,
    ],

    rules: {
	////////////////////////////////////////////////////////////////////////
	// Main building blocks: manuscript, block, paragraph, inline
	////////////////////////////////////////////////////////////////////////
	source_file: $ => seq(
	    field('tag', alias(':manuscript:', $.manuscript)),
	    field('meta', optional($.blockmeta)),
	    repeat($.blockcontent),
	    '::',
	    optional($.bibtex)),

	block: $ => prec(1, seq(
	    field('tag', $.blocktag),
	    field('meta', optional($.blockmeta)),
	    repeat($.blockcontent),
	    '::')),

	paragraph: $ => choice(
	    $.item,
	    $.caption,
	    seq(optional(seq(
		    field('tag', ':paragraph:'),
		    field('meta', $.inlinemeta))),
		repeat1($.paragraphcontent),
		alias($.paragraph_end, 'paragraph_end'))),

	inline: $ => prec(0, seq(
	    field('tag', $.inlinetag),
	    field('meta', optional($.inlinemeta)),
	    repeat($.inlinecontent),
	    '::')),

	construct: $ => seq(
	    field('tag', $.constructtag),
	    field('meta', optional($.inlinemeta)),
	    repeat($.inlinecontent),
	    '::'),

	////////////////////////////////////////////////////////////////////////
	// Special blocks, paragraphs, and inlines
	////////////////////////////////////////////////////////////////////////
	// Special blocks/inlines look just like normal blocks/inlines except that they
	// have some special parsing rule.  For example, $foo$ has the same meaning as
	// :math:foo::.

	specialblock: $ => choice(
	    $.table,

	    // The appendix is a 'stamp': it has no content and needs no Halmos
	    alias(':appendix:', $.appendix),

	    // The following are NOT stamps because they could have meta, though they
	    // cannot have content
	    seq(field('tag', alias(':bibliography:', $.bibliography)), '::'),
	    seq(field('tag', alias(':toc:', $.toc)), '::'),

	    // Sections have a special opening using hashtags, but their content is
	    // parsed just like any other block's.
	    seq(field('tag', alias(/# /, $.section)),
		field('title', $.text),
		field('meta', optional($.blockmeta)),
		repeat($.blockcontent),
		'::'),
	    seq(
		field('tag', alias(/## /, $.subsection)),
		field('title', $.text),
		field('meta', optional($.blockmeta)),
		repeat($.blockcontent),
		'::'),
	    seq(
		field('tag', alias(/### /, $.subsubsection)),
		field('title', $.text),
		field('meta', optional($.blockmeta)),
		repeat($.blockcontent),
		'::'),

	    // Math and code blocks have special open and close delimiters ($$) and
	    // (```) respesctively AND their content is taken as-is, i.e. they do not
	    // support recursive parsing.
	    seq(field('tag', alias(token(/\$\$/), $.mathblock)),
		field('meta', optional($.blockmeta)),
		alias($.asis_two_dollars_text, $.asis_text),
		alias(/\$\$/, "mathblock")),
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
	    repeat1($.paragraphcontent),
	    alias($.paragraph_end, 'paragraph_end')),

	item: $ => seq(
	    token(':item:'),
	    optional(field('meta', $.inlinemeta)),
	    repeat1($.paragraphcontent),
	    alias($.paragraph_end, 'paragraph_end')),

	specialinline: $ => choice(
	    // Prev* are special bc they have no content and no Halmos (they are stamps)
	    alias(token(':prev:'), $.prev),
	    alias(token(':prev2:'), $.prev2),
	    alias(token(':prev3:'), $.prev3),

	    // Math and code inlines have special open and close delimimters ($) and (`)
	    // respectively AND as-is content.
	    seq(field('tag', alias(token(/\$/), $.math)),
		field('meta', optional($.inlinemeta)),
		alias($.asis_dollar_text, $.asis_text),
		alias(/\$/, "math")),
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
		    repeat($.inlinecontent),
		    token('*'))),
	    prec.right(
		seq(field('tag', alias(token(/\//), $.spanemphas)),
		    repeat($.inlinecontent),
		    token('/'))),

	    // References (including 'previous'), citations, and URLs have standard
	    // delimiters but their content is parsed in a special way.
	    seq(field('tag', alias(token(':ref:'), $.ref)),
		field('target', alias(token(/[^,:]+/), $.text)),
		optional(seq(',', field('reftext', $.text))),
		'::'),
	    seq(field('tag', alias(token(':previous:'), $.previous)),
		field('target', alias(token(/[0-9]+/), $.text)),
		optional(seq(',', field('reftext', $.text))),
		'::'),
	    seq(field('tag', alias(token(':url:'), $.url)),
		field('target', alias(token(/(https:\/\/|http:\/\/)?[^\s,:]+?/), $.text)),
		optional(seq(',', field('reftext', $.text))),
		'::'),
	    seq(field('tag', alias(token(':cite:'), $.cite)),
		field('targetlabels', alias(token(/[^:]+/), $.text)),
		optional(seq(',', field('reftext', $.text))),
		'::'),
	),

	specialconstruct: $ => choice(
	    // stamp
	    alias(field("tag", ':qed:'), $.qed),
	),

	/////////////////////////////////////////////////////////////
	// Meta regions
	/////////////////////////////////////////////////////////////
	inlinemeta: $ => seq(
	    '{',
	    // an inline meta either contains a single pair, or a sequence of pairs with
	    // an ending comma plus a final pair without comma
	    choice(
		alias($.inlinemetapair, $.pair),
		seq(repeat1(seq(alias($.inlinemetapair, $.pair), ',')),
		    alias($.inlinemetapair, $.pair))),
	    '}'),

	blockmeta: $ => repeat1(alias($.pair, $.pair)),

	inlinemetapair: $ => choice(
	    seq($.metakey_text, alias($.metaval_text_inline, $.metaval_text)),
	    seq($.metakey_any, alias($.metaval_any_inline, $.metaval_any)),
            seq($.metakey_list, alias($.metaval_list_inline, $.metaval_list)),
	    $.metakey_bool),

	pair: $ => choice(
	    seq($.metakey_text, $.metaval_text),
	    seq($.metakey_any, $.metaval_any),
            seq($.metakey_list, $.metaval_list),
	    $.metakey_bool),

	/////////////////////////////////////////////////////////////
	// Meta pair types
	/////////////////////////////////////////////////////////////
	metaval_any: $ => alias(token(/[^\S\r\n]*.+?\n/), 'text'),

	metaval_any_inline: $ => alias(token(/[^\S\r\n]*[^,}\n]+?\n/), 'text'),

	metaval_text: $ => alias($.text, 'text'),

	metaval_text_inline: $ => alias($.upto_brace_or_comma_text, 'text'),

        metaval_list: $ => choice(
	    seq('{',
                repeat1(seq(alias($.upto_brace_or_comma_text, $.metaval_list_item), ',')),
                alias($.upto_brace_or_comma_text, $.metaval_list_item),
                '}'),
	    alias($.text, $.metaval_list_item)
        ),

	metaval_list_inline: $ => choice(
            seq('{',
                repeat1(seq(alias($.upto_brace_or_comma_text, $.metaval_list_item), ',')),
                alias($.upto_brace_or_comma_text, $.metaval_list_item),
                '}'),
	    alias($.upto_brace_or_comma_text, $.metaval_list_item),
	),

	/////////////////////////////////////////////////////////////
	// Tables
	/////////////////////////////////////////////////////////////
	table: $ => seq(
	    field('tag', ':table:'),
	    field('meta', optional($.blockmeta)),
	    field('head', optional($.thead)),
	    field('body', optional($.tbody)),
	    field('caption', optional($.caption)),
	    '::'),

	thead: $ => seq(
	    field('tag', ':thead:'),
	    repeat1(choice($.tr, $.trshort)),
	    '::'),

	tbody: $ => seq(
	    field('tag', ':tbody:'),
	    repeat1(choice($.tr, $.trshort)),
	    '::'),

	tr: $ => seq(
	    field('tag', ':tr:'),
	    repeat1($.td),
	    '::'),

	trshort: $ => prec(1,
			   seq(field('tag', ':tr:'),
			       choice($.tdcontent, seq(repeat1(seq($.tdcontent, ':')), $.tdcontent)),
			       '::')),

	td: $ => seq(
	    field('tag', token(':td:')),
	    field('meta', optional($.inlinemeta)),
	    $.tdcontent,
	    '::'),

	tdcontent: $ => repeat1($.inlinecontent),


	/////////////////////////////////////////////////////////////
	// Bibliography stuff
	/////////////////////////////////////////////////////////////
	bibtex: $ => seq(':bibtex:', repeat($.bibitem), '::'),

	bibitem: $ => seq(
	    '@',
	    field('kind', alias(token(/book|article|software/), $.kind)),
	    '{',
	    field('label', alias(token(/[^,]+?/), $.label)),
	    ',',
	    choice(
		$.bibitempair,
		seq(repeat1(seq($.bibitempair, ',')), $.bibitempair, optional(','))),
	    '}'),

	bibitempair: $ => seq(
	    alias(/title|author|year|publisher|journal|volume|number|doi|url|edition/, $.key),
	    '=',
	    '{',
	    alias(/[^}]+?/, $.value),
	    '}'),

	/////////////////////////////////////////////////////////////
	// Content choices
	/////////////////////////////////////////////////////////////
	blockcontent: $ => choice($.specialblock, $.block, $.paragraph),

	paragraphcontent: $ => choice($.specialinline, $.inline, $.specialconstruct, $.construct, $.text),

	inlinecontent: $ => choice($.specialinline, $.inline, $.specialconstruct, $.construct, $.text),

	/////////////////////////////////////////////////////////////
	// Tag choices
	/////////////////////////////////////////////////////////////
	constructtag: $ => choice(
	    alias(':assume:', $.assume),
	    alias(':suppose:', $.suppose),
	    alias(':prove:', $.prove),
	    alias(':then:', $.then),
	    alias(':new:', $.new),
	    alias(':let:', $.let),
	    alias(':case:', $.case),
	    alias(':define:', $.define),
	    alias(':write:', $.write),
	    alias(':wlog:', $.wlog),
	    alias(':suffices:', $.suffices),
	    alias(':claim:', $.claim),
	    alias(':|-:', $.claim),
	    alias(':⊢:', $.claim),
	    alias(':pick:', $.pick),
	    alias(':st:', $.st),
	    // alias(':qed:', $.qed), // should be a stamp
	),

	inlinetag: $ => choice(
	    alias(':draft:', $.draft),
	    alias(':note:', $.note),
	    alias(':span:', $.span),
	),

	blocktag: $ => choice(
	    alias(':abstract:', $.abstract),
	    alias(':author:', $.author),
	    alias(':definition:', $.definition),
	    alias(':corollary:', $.corollary),
	    alias(':enumerate:', $.enumerate),
	    alias(':example:', $.example),
	    alias(':itemize:', $.itemize),
	    alias(':lemma:', $.lemma),
	    alias(':figure:', $.figure),
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
	    alias(':theorem:', $.theorem),
	),

	metakey_text: $ => choice(
	    alias(':affiliation:', $.affiliation),
	    alias(':email:', $.email),
	    alias(':label:', $.label),
	    alias(':name:', $.name),
	    alias(':reftext:', $.reftext),
	    alias(':title:', $.title),
	    alias(':goal:', $.goal),
            alias(':lang:', $.lang),
	),

	metakey_bool: $ => choice(
	    alias(':nonum:', $.nonum),
	    alias(':strong:', $.strong),
	    alias(':emphas:', $.emphas),
            alias(':isclaim:', $.isclaim),
	),

        metakey_list: $ => choice(
	    alias(':keywords:', $.keywords),
	    alias(':msc:', $.msc),
	    alias(':types:', $.types),
	),

	metakey_any: $ => choice(
	    alias(':date:', $.date),
	    alias(':path:', $.path),
            alias(':scale:', $.scale),
	),

	// It is important that comment appears at the end so that other rules will be
	// given automatic precedence.  Note this is not possible by simply using prec()
	// since the (/.*/) part of a comment will match anything and tree-sitter pays
	// attentions to the length of matches to determine precedence.
	comment: $ => token(seq('%', /.*/)),

    }
});
