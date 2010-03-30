;; emacs org-mode batch configuration

;; local emacs extensions
(add-to-list 'load-path "./elisp")

;;; ORG mode
(require 'org-install)
(require 'org-exp-blocks)
(require 'htmlize)

;; set auto load on .org files
(add-to-list 'auto-mode-alist '("\\.org\\'" . org-mode))

;; org mode customisations
(setq org-export-htmlize-output-type 'css)


'(org-export-blocks (quote ((comment org-export-blocks-format-comment t) (ditaa org-export-blocks-format-ditaa nil) (dot org-export-blocks-format-dot t) (r org-export-blocks-format-R nil) (R org-export-blocks-format-R nil))))
'(org-export-html-inline-images t)
'(org-export-html-use-infojs t)


'(org-export-htmlize-output-type "css")
'(org-export-html-validation-link "<p class=\"xhtml-validation\"><a href=\"http://validator.w3.org/check?uri=referer\">Validate XHTML 1.0</a></p>")
'(org-export-html-with-timestamp nil)
'(org-modules (quote (org-bbdb org-bibtex org-info org-jsinfo org-irc org-w3m org-mouse org-eval org-eval-light org-exp-bibtex org-man org-mtags org-panel org-R org-special-blocks org-exp-blocks)))

