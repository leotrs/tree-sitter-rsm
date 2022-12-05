;; Labels
(blockmeta
 (blockmetapair
  (metatag_text
   (label))
  (metavalue_text) @label))
(inlinemeta
 (inlinemetapair
  (metatag_text
   (label))
  (metavalue_text_inline
   (upto_brace_or_comma_text)) @label))
(specialinline
 tag: (ref)
 target: (text) @label
 reftext: (text))
(specialinline
 tag: (ref)
 target: (text) @label)
(specialinline
 tag: (cite)
 targetlabels: (text) @label)
(bibitem
 (label) @label)

;; Tags
[
 (author)
 (abstract)
 (toc)
 (theorem)
 (lemma)
 (remark)
 (proposition)
 (proof)
 (step)
 (subproof)
 (sketch)
 (bibliography)
 (figure)
 (claimblock)
 (algorithm)
 (enumerate)
 (itemize)
 (definition)
 ] @block-tag

[
 (claim)
 (draft)
 (note)
 (span)
 (cite)
 (ref)
 (prev)
 (prev2)
 (prev3)
 (previous)
 (url)
 ] @inline-tag

[
 (label)
 (types)
 (title)
 (path)
 (scale)
 (affiliation)
 (email)
 (name)
 (reftext)
 (nonum)
 (strong)
 (emphas)
 (keywords)
 (MSC)
 ] @meta-tag

(paragraph (":paragraph:") @inline-tag)
(item (":item:") @inline-tag)
(caption (":caption:") @inline-tag)

;; Special regions
(asis_text) @asis

;; Bibliography stuff
(bibtex (":bibtex:") @block-tag)
(bibitem (kind) @block-tag)
(bibitem (kind) @block-tag)
(bibitempair
 (key) @meta-tag
 ("=") @special-delim
 ("{") @special-delim
 (value)
 ("}") @special-delim)
(bibtex ("::") @block-halmos)

;; Table
(table (":table:") @block-tag)
(tbody (":tbody:") @block-tag)
(thead (":thead:") @block-tag)
(table ("::") @block-halmos)
(tbody ("::") @block-halmos)
(thead ("::") @block-halmos)
(tr (":tr:") @inline-tag)
(td (":td:") @inline-tag)
(trshort (":tr:") @inline-tag)
(tr ("::") @inline-halmos)
(td ("::") @inline-halmos)
(trshort ("::") @inline-halmos)
(trshort (tdcontent) . (":") @trshort-colon . (tdcontent))

;; Delimiters
[(mathblock) "mathblock"] @special-delim
[(math) "math"] @special-delim
;; [(codeblock) "codeblock"] @special-delim
;; [(code) "code"] @special-delim
(inlinemeta ("{") @meta-delim)
(inlinemeta ("}") @meta-delim)
((specialinline
  (claimshort) @inline-tag)
 (text) @inline-halmos)

;; Span properties
(specialinline (spanemphas) (text)) @emphas
(specialinline (spanstrong) (text)) @strong

;; Sections
(manuscript) @heading
(specialblock
 tag: [(section) (subsection) (subsubsection)]
 title: (text) @heading)
((metatag_text (title)) (metavalue_text) @heading)
(specialblock [(section) (subsection) (subsubsection)] @heading)

;; Halmos
(inline ("::") @inline-halmos)
(specialinline ("::") @inline-halmos)

(block ("::") @block-halmos)
(specialblock [(section) (subsection) (subsubsection)] ("::") @heading-halmos .)
(source_file ("::") @heading-halmos)

;; Draft
(inline
 (inlinetag
  (draft))
 (text) @draft)

;; Comments
(comment) @comment

;; Errors
(ERROR) @error
