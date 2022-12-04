[
 (manuscript)
 (author)
 (abstract)
 (toc)
 ;; (section)
 (theorem)
 (lemma)
 (remark)
 (proposition)
 (proof)
 (step)
 (subproof)
 (sketch)
 (bibliography)
 (bibtex)
 (figure)
 (claimblock)
 (algorithm)
 (enumerate)
 (itemize)
 (definition)
 (table)
 (tbody)
 (thead)
 ] @blocktag

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
 ] @inlinetag

[
 (label)
 (types)
 (path)
 (affiliation)
 (email)
 (name)
 (reftext)
 (nonum)
 (strong)
 (emphas)
 (keywords)
 (MSC)
 ] @metatag

"::" @halmos

":" @colon

[(mathblock) "mathblock"] @mathblock

[(math) "math"] @math

;; (codeblock) @codeblock
;; "codeblock" @codeblock

(specialinline (spanemphas) (text)) @emphas
(specialinline (spanstrong) (text)) @strong

(specialblock
 tag: [(section) (subsection) (subsubsection)]
 title: (text) @heading)

((metatag_text (title)) (metavalue_text) @heading)
