;; Twilight Colour Theme for Emacs.
;;
;; Defines a colour scheme resembling that of the original TextMate Twilight colour theme.
;; To use add the following to your .emacs file (requires the color-theme package):
;;
;; (require 'color-theme)
;; (color-theme-initialize)
;; (load-file "~/.emacs.d/twilight-emacs/color-theme-twilight.el")
;;
;; And then (color-theme-twilight) to activate it.
;;
;; Several areas still require improvement such as recognition of code that ruby-mode doesn't
;; yet pick up (eg. parent classes), Rails/Merb keywords, or non Ruby code related areas
;; (eg. dired, HTML, etc). Please feel free to customize further and send in any improvements,
;; patches most welcome.
;;
;; MIT License Copyright (c) 2008 Marcus Crafter <crafterm@redartisan.com>
;; Credits due to the excellent TextMate Twilight theme
;;
;; Thanks to Travis Jeffery for ido-mode and fixes to the minibuffer-prompt to fit in with the rest of the theme
;;

(defun color-theme-twilight ()
  "Color theme by Marcus Crafter, based off the TextMate Twilight theme, created 2008-04-18"
  (interactive)
  (color-theme-install
	'(color-theme-twilight
	  ((background-color . "#141414")
		(background-mode . dark)
		(border-color . "black")
		(cursor-color . "#A7A7A7")
		(foreground-color . "#F8F8F8")
		(mouse-color . "sienna1"))
	  (default ((t (:background "#141414" :foreground "#CACACA"))))
	  (blue ((t (:foreground "blue"))))
	  (border-glyph ((t (nil))))
	  (buffers-tab ((t (:background "#141414" :foreground "#CACACA"))))
	  (font-lock-builtin-face ((t (:foreground "#CACACA"))))
	  (font-lock-comment-face ((t (:foreground "#5F5A60"))))
	  (font-lock-constant-face ((t (:foreground "#CF6A4C"))))
	  (font-lock-doc-string-face ((t (:foreground "DarkOrange"))))
	  (font-lock-function-name-face ((t (:foreground "#9B703F"))))
	  (font-lock-keyword-face ((t (:foreground "#CDA869"))))
	  (font-lock-preprocessor-face ((t (:foreground "Aquamarine"))))
	  (font-lock-reference-face ((t (:foreground "SlateBlue"))))

	  (font-lock-regexp-grouping-backslash ((t (:foreground "#E9C062"))))
	  (font-lock-regexp-grouping-construct ((t (:foreground "red"))))

	  (minibuffer-prompt ((t (:foreground "#5F5A60"))))
	  (ido-subdir ((t (:foreground "#CF6A4C"))))
	  (ido-first-match ((t (:foreground "#8F9D6A"))))
	  (ido-only-match ((t (:foreground "#8F9D6A"))))
	  (mumamo-background-chunk-submode ((t (:background "#222222")))) 

	  (font-lock-string-face ((t (:foreground "#8F9D6A"))))
	  (font-lock-type-face ((t (:foreground "#9B703F"))))
	  (font-lock-variable-name-face ((t (:foreground "#7587A6"))))
	  (font-lock-warning-face ((t (:background "#EE799F" :foreground "red"))))
	  (gui-element ((t (:background "#D4D0C8" :foreground "black"))))
	  (region ((t (:background "#27292A"))))
	  (mode-line ((t (:background "grey75" :foreground "black"))))
	  (highlight ((t (:background "#111111"))))
	  (highline-face ((t (:background "SeaGreen"))))
	  (left-margin ((t (nil))))
	  (text-cursor ((t (:background "yellow" :foreground "black"))))
	  (toolbar ((t (nil))))
	  (underline ((nil (:underline nil))))
	  (zmacs-region ((t (:background "snow" :foreground "blue")))))))
