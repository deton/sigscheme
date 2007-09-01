;;  Filename : test-assoc.scm
;;  About    : unit tests for assq, assv, assoc
;;
;;  Copyright (C) 2006 YAMAMOTO Kengo <yamaken AT bp.iij4u.or.jp>
;;  Copyright (c) 2007 SigScheme Project <uim-en AT googlegroups.com>
;;
;;  All rights reserved.
;;
;;  Redistribution and use in source and binary forms, with or without
;;  modification, are permitted provided that the following conditions
;;  are met:
;;
;;  1. Redistributions of source code must retain the above copyright
;;     notice, this list of conditions and the following disclaimer.
;;  2. Redistributions in binary form must reproduce the above copyright
;;     notice, this list of conditions and the following disclaimer in the
;;     documentation and/or other materials provided with the distribution.
;;  3. Neither the name of authors nor the names of its contributors
;;     may be used to endorse or promote products derived from this software
;;     without specific prior written permission.
;;
;;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
;;  IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
;;  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
;;  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
;;  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
;;  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
;;  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
;;  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
;;  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;;  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
;;  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

(require-extension (unittest))

(define tn test-name)

(define elm0 (lambda () 0))
(define elm1 (lambda () 1))
(define elm2 (lambda () 2))
(define elm3 (lambda () 3))
(define nil  '())
(define cdr3 (cons (cons elm3 3) nil))
(define cdr2 (cons (cons elm2 2) cdr3))
(define cdr1 (cons (cons elm1 1) cdr2))
(define cdr0 (cons (cons elm0 0) cdr1))
(define alist cdr0)

;; Remake char object to avoid constant optimization. If the implementation
;; does not have neither immediate char nor preallocated char objects, (eq? c
;; (char c)) will be false.
(define char
  (lambda (c)
    (integer->char (char->integer c))))

;;
;; assq
;;

(tn "assq symbols")
(assert-error  (tn) (lambda () (assq 'a '(a))))
(assert-error  (tn) (lambda () (assq 'a '((A . 0) a))))
(assert-false  (tn)            (assq 'a '()))
(assert-equal? (tn) '(a . 0)   (assq 'a '((a . 0))))
(assert-false  (tn)            (assq 'b '((a . 0))))
(assert-equal? (tn) '(A . 0)   (assq 'A '((A . 0) (a . 1) (b . 2))))
(assert-equal? (tn) '(a . 1)   (assq 'a '((A . 0) (a . 1) (b . 2))))
(assert-equal? (tn) '(b . 2)   (assq 'b '((A . 0) (a . 1) (b . 2))))
(assert-false  (tn)            (assq 'c '((A . 0) (a . 1) (b . 2))))
(tn "assq builtin procedures")
(assert-false  (tn)            (assq + (list)))
(assert-equal? (tn) (cons + 0) (assq + (list (cons + 0))))
(assert-false  (tn)            (assq - (list (cons + 0))))
(assert-equal? (tn) (cons + 0) (assq + (list (cons + 0) (cons - 1) (cons * 2))))
(assert-equal? (tn) (cons - 1) (assq - (list (cons + 0) (cons - 1) (cons * 2))))
(assert-equal? (tn) (cons * 2) (assq * (list (cons + 0) (cons - 1) (cons * 2))))
(assert-false  (tn)            (assq / (list (cons + 0) (cons - 1) (cons * 2))))
(tn "assq closures")
(assert-equal? (tn) (car cdr3) (assq elm3 alist))
(assert-equal? (tn) (car cdr2) (assq elm2 alist))
(assert-equal? (tn) (car cdr1) (assq elm1 alist))
(assert-equal? (tn) (car cdr0) (assq elm0 alist))
(assert-false  (tn)            (assq (lambda() #f) alist))
(tn "assq strings with non-constant key")
;; These tests assume that (string #\a) is not optimized as constant string.
(assert-false  (tn) (assq (string #\a) '()))
(assert-false  (tn) (assq (string #\a) '(("a" . a))))
(assert-false  (tn) (assq (string #\b) '(("a" . a))))
(assert-false  (tn) (assq (string #\a) '(("a" . a) ("b" . b) ("c" . c))))
(assert-false  (tn) (assq (string #\b) '(("a" . a) ("b" . b) ("c" . c))))
(assert-false  (tn) (assq (string #\c) '(("a" . a) ("b" . b) ("c" . c))))
(assert-false  (tn) (assq (string #\d) '(("a" . a) ("b" . b) ("c" . c))))
(tn "assq lists with non-constant key")
;; These tests assume that the keys are not optimized as constant object.
(assert-false  (tn) (assq (list (string #\a)) '()))
(assert-false  (tn) (assq (list (string #\a)) '((("a") . a))))
(assert-false  (tn) (assq (list (string #\b)) '((("a") . a))))
(assert-false  (tn) (assq (list (string #\a))
                          '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn) (assq (list (string #\b))
                          '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn) (assq (list (string #\c))
                          '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn) (assq (list (string #\d))
                          '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn) (assq (list (string #\a #\B #\c)
                                (list (string #\d) (list (string #\e))))
                          '((("aBc" ("d" ("E"))) 0)
                            (("aBc" ("d" ("e"))) 1)
                            ("f")
                            ("g"))))

(tn "assq improper lists: symbols")
(assert-error  (tn) (lambda () (assq 'a 'a)))
(assert-equal? (tn) '(a . 1)   (assq 'a '((A . 0) (a . 1) (b . 2) . 3)))
(assert-error  (tn) (lambda () (assq 'c '((A . 0) (a . 1) (b . 2) . 3))))
(tn "assq improper lists: builtin procedures")
(assert-error  (tn) (lambda () (assq '+ '+)))
(assert-equal? (tn) '(- . 1)   (assq '- '((+ . 0) (- . 1) (* . 2) . 3)))
(assert-error  (tn) (lambda () (assq '/ '((+ . 0) (- . 1) (* . 2) . 3))))
(tn "assq improper lists: strings")
(assert-error  (tn) (lambda () (assq (string #\b)
                                     '(("a" . 0) ("b" . 1) ("c" . 2) . 3))))
(tn "assq improper lists: lists")
(assert-error  (tn) (lambda ()
                      (assq (list (string #\b))
                            '((("a") . 0) (("b") . 1) (("c") . 2) . 3))))

(tn "assq from R5RS examples")
(define e '((a 1) (b 2) (c 3)))
(assert-equal? (tn) '(a 1) (assq 'a e))
(assert-equal? (tn) '(b 2) (assq 'b e))
(assert-false  (tn) (assq 'd e))
(assert-false  (tn) (assq (list 'a) '(((a)) ((b)) ((c)))))

;;
;; assv
;;

(tn "assv symbols")
(assert-error  (tn) (lambda () (assv 'a '(a))))
(assert-error  (tn) (lambda () (assv 'a '((A . 0) a))))
(assert-false  (tn)            (assv 'a '()))
(assert-equal? (tn) '(a . 0)   (assv 'a '((a . 0))))
(assert-false  (tn)            (assv 'b '((a . 0))))
(assert-equal? (tn) '(A . 0)   (assv 'A '((A . 0) (a . 1) (b . 2))))
(assert-equal? (tn) '(a . 1)   (assv 'a '((A . 0) (a . 1) (b . 2))))
(assert-equal? (tn) '(b . 2)   (assv 'b '((A . 0) (a . 1) (b . 2))))
(assert-false  (tn)            (assv 'c '((A . 0) (a . 1) (b . 2))))
(tn "assv builtin procedures")
(assert-false  (tn)            (assv + (list)))
(assert-equal? (tn) (cons + 0) (assv + (list (cons + 0))))
(assert-false  (tn)            (assv - (list (cons + 0))))
(assert-equal? (tn) (cons + 0) (assv + (list (cons + 0) (cons - 1) (cons * 2))))
(assert-equal? (tn) (cons - 1) (assv - (list (cons + 0) (cons - 1) (cons * 2))))
(assert-equal? (tn) (cons * 2) (assv * (list (cons + 0) (cons - 1) (cons * 2))))
(assert-false  (tn)            (assv / (list (cons + 0) (cons - 1) (cons * 2))))
(tn "assv closures")
(assert-equal? (tn) (car cdr3) (assv elm3 alist))
(assert-equal? (tn) (car cdr2) (assv elm2 alist))
(assert-equal? (tn) (car cdr1) (assv elm1 alist))
(assert-equal? (tn) (car cdr0) (assv elm0 alist))
(assert-false  (tn)            (assv (lambda() #f) alist))
(tn "assv numbers")
(assert-false  (tn)            (assv 0 '()))
(assert-equal? (tn) '(0 . a)   (assv 0 '((0 . a))))
(assert-false  (tn)            (assv 1 '((0 . a))))
(assert-equal? (tn) '(0 . a)   (assv 0 '((0 . a) (1 . b) (2 . c))))
(assert-equal? (tn) '(1 . b)   (assv 1 '((0 . a) (1 . b) (2 . c))))
(assert-equal? (tn) '(2 . c)   (assv 2 '((0 . a) (1 . b) (2 . c))))
(assert-false  (tn)            (assv 3 '((0 . a) (1 . b) (2 . c))))
(assert-equal? (tn) '(5 7)     (assv 5 '((2 3) (5 7) (11 13))))  ;; R5RS
(tn "assv chars")
(assert-false  (tn)            (assv #\a '()))
(assert-equal? (tn) '(#\a . a) (assv #\a '((#\a . a))))
(assert-false  (tn)            (assv #\b '((#\a . a))))
(assert-equal? (tn) '(#\a . a) (assv #\a '((#\a . a) (#\b . b) (#\c . c))))
(assert-equal? (tn) '(#\b . b) (assv #\b '((#\a . a) (#\b . b) (#\c . c))))
(assert-equal? (tn) '(#\c . c) (assv #\c '((#\a . a) (#\b . b) (#\c . c))))
(assert-false  (tn)            (assv #\d '((#\a . a) (#\b . b) (#\c . c))))
(tn "assv chars with non-constant key")
(assert-false  (tn)            (assv (char #\a) '()))
(assert-equal? (tn) '(#\a . a) (assv (char #\a) '((#\a . a))))
(assert-false  (tn)            (assv (char #\b) '((#\a . a))))
(assert-equal? (tn) '(#\a . a) (assv (char #\a) '((#\a . a) (#\b . b) (#\c . c))))
(assert-equal? (tn) '(#\b . b) (assv (char #\b) '((#\a . a) (#\b . b) (#\c . c))))
(assert-equal? (tn) '(#\c . c) (assv (char #\c) '((#\a . a) (#\b . b) (#\c . c))))
(assert-false  (tn)            (assv (char #\d) '((#\a . a) (#\b . b) (#\c . c))))
(tn "assv strings with non-constant key")
;; These tests assume that (string #\a) is not optimized as constant string.
(assert-false  (tn) (assv (string #\a) '()))
(assert-false  (tn) (assv (string #\a) '(("a" . a))))
(assert-false  (tn) (assv (string #\b) '(("a" . a))))
(assert-false  (tn) (assv (string #\a) '(("a" . a) ("b" . b) ("c" . c))))
(assert-false  (tn) (assv (string #\b) '(("a" . a) ("b" . b) ("c" . c))))
(assert-false  (tn) (assv (string #\c) '(("a" . a) ("b" . b) ("c" . c))))
(assert-false  (tn) (assv (string #\d) '(("a" . a) ("b" . b) ("c" . c))))
(tn "assv lists with non-constant key")
;; These tests assume that the keys are not optimized as constant object.
(assert-false  (tn) (assv (list (string #\a)) '()))
(assert-false  (tn) (assv (list (string #\a)) '((("a") . a))))
(assert-false  (tn) (assv (list (string #\b)) '((("a") . a))))
(assert-false  (tn) (assv (list (string #\a))
                          '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn) (assv (list (string #\b))
                          '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn) (assv (list (string #\c))
                          '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn) (assv (list (string #\d))
                          '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn) (assv (list (string #\a #\B #\c)
                                (list (string #\d) (list (string #\e))))
                          '((("aBc" ("d" ("E"))) 0)
                            (("aBc" ("d" ("e"))) 1)
                            ("f")
                            ("g"))))

(tn "assv improper lists: symbols")
(assert-error  (tn) (lambda () (assv 'a 'a)))
(assert-equal? (tn) '(a . 1)   (assv 'a '((A . 0) (a . 1) (b . 2) . 3)))
(assert-error  (tn) (lambda () (assv 'c '((A . 0) (a . 1) (b . 2) . 3))))
(tn "assv improper lists: builtin procedures")
(assert-error  (tn) (lambda () (assv '+ '+)))
(assert-equal? (tn) '(- . 1)   (assv '- '((+ . 0) (- . 1) (* . 2) . 3)))
(assert-error  (tn) (lambda () (assv '/ '((+ . 0) (- . 1) (* . 2) . 3))))
(tn "assv improper lists: numbers")
(assert-error  (tn) (lambda () (assv 0 '0)))
(assert-equal? (tn) '(1 . b)   (assv 1 '((0 . a) (1 . b) (3 . c) . d)))
(assert-error  (tn) (lambda () (assv 4 '((0 . a) (1 . b) (3 . c) . d))))
(tn "assv improper lists: chars")
(assert-error  (tn) (lambda () (assv #\a #\a)))
(assert-equal? (tn) '(#\b . 1) (assv #\b
                                     '((#\a . 0) (#\b . 1) (#\c . 2) . 3)))
(assert-equal? (tn) '(#\b . 1) (assv (char #\b)
                                     '((#\a . 0) (#\b . 1) (#\c . 2) . 3)))
(assert-error  (tn) (lambda () (assv #\d
                                     '((#\a . 0) (#\b . 1) (#\c . 2) . 3))))
(tn "assv improper lists: strings")
(assert-error  (tn) (lambda () (assv (string #\b)
                                     '(("a" . 0) ("b" . 1) ("c" . 2) . 3))))
(tn "assv improper lists: lists")
(assert-error  (tn) (lambda ()
                      (assv (list (string #\b))
                            '((("a") . 0) (("b") . 1) (("c") . 2) . 3))))

;;
;; assoc
;;

(tn "assoc symbols")
(assert-error  (tn) (lambda () (assoc 'a '(a))))
(assert-error  (tn) (lambda () (assoc 'a '((A . 0) a))))
(assert-false  (tn)            (assoc 'a '()))
(assert-equal? (tn) '(a . 0)   (assoc 'a '((a . 0))))
(assert-false  (tn)            (assoc 'b '((a . 0))))
(assert-equal? (tn) '(A . 0)   (assoc 'A '((A . 0) (a . 1) (b . 2))))
(assert-equal? (tn) '(a . 1)   (assoc 'a '((A . 0) (a . 1) (b . 2))))
(assert-equal? (tn) '(b . 2)   (assoc 'b '((A . 0) (a . 1) (b . 2))))
(assert-false  (tn)            (assoc 'c '((A . 0) (a . 1) (b . 2))))
(tn "assoc builtin procedures")
(assert-false  (tn)            (assoc + (list)))
(assert-equal? (tn) (cons + 0) (assoc + (list (cons + 0))))
(assert-false  (tn)            (assoc - (list (cons + 0))))
(assert-equal? (tn) (cons + 0) (assoc + (list (cons + 0) (cons - 1) (cons * 2))))
(assert-equal? (tn) (cons - 1) (assoc - (list (cons + 0) (cons - 1) (cons * 2))))
(assert-equal? (tn) (cons * 2) (assoc * (list (cons + 0) (cons - 1) (cons * 2))))
(assert-false  (tn)            (assoc / (list (cons + 0) (cons - 1) (cons * 2))))
(tn "assoc closures")
(assert-equal? (tn) (car cdr3) (assoc elm3 alist))
(assert-equal? (tn) (car cdr2) (assoc elm2 alist))
(assert-equal? (tn) (car cdr1) (assoc elm1 alist))
(assert-equal? (tn) (car cdr0) (assoc elm0 alist))
(assert-false  (tn)            (assoc (lambda() #f) alist))
(tn "assoc numbers")
(assert-false  (tn)            (assoc 0 '()))
(assert-equal? (tn) '(0 . a)   (assoc 0 '((0 . a))))
(assert-false  (tn)            (assoc 1 '((0 . a))))
(assert-equal? (tn) '(0 . a)   (assoc 0 '((0 . a) (1 . b) (2 . c))))
(assert-equal? (tn) '(1 . b)   (assoc 1 '((0 . a) (1 . b) (2 . c))))
(assert-equal? (tn) '(2 . c)   (assoc 2 '((0 . a) (1 . b) (2 . c))))
(assert-false  (tn)            (assoc 3 '((0 . a) (1 . b) (2 . c))))
(assert-equal? (tn) '(5 7)     (assoc 5 '((2 3) (5 7) (11 13))))  ;; R5RS
(tn "assoc chars")
(assert-false  (tn)            (assoc #\a '()))
(assert-equal? (tn) '(#\a . a) (assoc #\a '((#\a . a))))
(assert-false  (tn)            (assoc #\b '((#\a . a))))
(assert-equal? (tn) '(#\a . a) (assoc #\a '((#\a . a) (#\b . b) (#\c . c))))
(assert-equal? (tn) '(#\b . b) (assoc #\b '((#\a . a) (#\b . b) (#\c . c))))
(assert-equal? (tn) '(#\c . c) (assoc #\c '((#\a . a) (#\b . b) (#\c . c))))
(assert-false  (tn)            (assoc #\d '((#\a . a) (#\b . b) (#\c . c))))
(tn "assoc chars with non-constant key")
(assert-false  (tn)            (assoc (char #\a) '()))
(assert-equal? (tn) '(#\a . a) (assoc (char #\a) '((#\a . a))))
(assert-false  (tn)            (assoc (char #\b) '((#\a . a))))
(assert-equal? (tn) '(#\a . a) (assoc (char #\a) '((#\a . a) (#\b . b) (#\c . c))))
(assert-equal? (tn) '(#\b . b) (assoc (char #\b) '((#\a . a) (#\b . b) (#\c . c))))
(assert-equal? (tn) '(#\c . c) (assoc (char #\c) '((#\a . a) (#\b . b) (#\c . c))))
(assert-false  (tn)            (assoc (char #\d) '((#\a . a) (#\b . b) (#\c . c))))
(tn "assoc strings")
(assert-false  (tn)            (assoc "a" '()))
(assert-equal? (tn) '("a" . a) (assoc "a" '(("a" . a))))
(assert-false  (tn)            (assoc "b" '(("a" . a))))
(assert-equal? (tn) '("a" . a) (assoc "a" '(("a" . a) ("b" . b) ("c" . c))))
(assert-equal? (tn) '("b" . b) (assoc "b" '(("a" . a) ("b" . b) ("c" . c))))
(assert-equal? (tn) '("c" . c) (assoc "c" '(("a" . a) ("b" . b) ("c" . c))))
(assert-false  (tn)            (assoc "d" '(("a" . a) ("b" . b) ("c" . c))))
(tn "assoc strings with non-constant key")
;; These tests assume that (string #\a) is not optimized as constant string.
(assert-false  (tn)            (assoc (string #\a) '()))
(assert-equal? (tn) '("a" . a) (assoc (string #\a) '(("a" . a))))
(assert-false  (tn)            (assoc (string #\b) '(("a" . a))))
(assert-equal? (tn) '("a" . a) (assoc (string #\a)
                                      '(("a" . a) ("b" . b) ("c" . c))))
(assert-equal? (tn) '("b" . b) (assoc (string #\b)
                                      '(("a" . a) ("b" . b) ("c" . c))))
(assert-equal? (tn) '("c" . c) (assoc (string #\c)
                                      '(("a" . a) ("b" . b) ("c" . c))))
(assert-false  (tn)            (assoc (string #\d)
                                      '(("a" . a) ("b" . b) ("c" . c))))
(tn "assoc lists")
;; These tests assume that the keys are not optimized as constant object.
(assert-false  (tn)              (assoc '("a") '()))
(assert-equal? (tn) '(("a") . a) (assoc '("a") '((("a") . a))))
(assert-false  (tn)              (assoc '("b") '((("a") . a))))
(assert-equal? (tn) '(("a") . a) (assoc '("a") '((("a") . a) (("b") . b) (("c") . c))))
(assert-equal? (tn) '(("b") . b) (assoc '("b") '((("a") . a) (("b") . b) (("c") . c))))
(assert-equal? (tn) '(("c") . c) (assoc '("c") '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn)              (assoc '("d") '((("a") . a) (("b") . b) (("c") . c))))
(assert-equal? (tn)
               '(("aBc" ("d" ("e"))) 1)
               (assoc '("aBc" ("d" ("e")))
                      '((("aBc" ("d" ("E"))) 0)
                        (("aBc" ("d" ("e"))) 1)
                        ("f")
                        ("g"))))
(tn "assoc lists with non-constant key")
(assert-false  (tn)              (assoc (list (string #\a)) '()))
(assert-equal? (tn) '(("a") . a) (assoc (list (string #\a)) '((("a") . a))))
(assert-false  (tn)              (assoc (list (string #\b)) '((("a") . a))))
(assert-equal? (tn) '(("a") . a) (assoc (list (string #\a)) '((("a") . a) (("b") . b) (("c") . c))))
(assert-equal? (tn) '(("b") . b) (assoc (list (string #\b)) '((("a") . a) (("b") . b) (("c") . c))))
(assert-equal? (tn) '(("c") . c) (assoc (list (string #\c)) '((("a") . a) (("b") . b) (("c") . c))))
(assert-false  (tn)              (assoc (list (string #\d)) '((("a") . a) (("b") . b) (("c") . c))))
(assert-equal? (tn)
               '(("aBc" ("d" ("e"))) 1)
               (assoc (list (string #\a #\B #\c)
                            (list (string #\d) (list (string #\e))))
                      '((("aBc" ("d" ("E"))) 0)
                        (("aBc" ("d" ("e"))) 1)
                        ("f")
                        ("g"))))
(assert-equal? (tn) '((a)) (assoc (list 'a) '(((a)) ((b)) ((c)))))  ;; R5RS

(tn "assoc improper lists: symbols")
(assert-error  (tn) (lambda () (assoc 'a 'a)))
(assert-equal? (tn) '(a . 1)   (assoc 'a '((A . 0) (a . 1) (b . 2) . 3)))
(assert-error  (tn) (lambda () (assoc 'c '((A . 0) (a . 1) (b . 2) . 3))))
(tn "assoc improper lists: builtin procedures")
(assert-error  (tn) (lambda () (assoc '+ '+)))
(assert-equal? (tn) '(- . 1)   (assoc '- '((+ . 0) (- . 1) (* . 2) . 3)))
(assert-error  (tn) (lambda () (assoc '/ '((+ . 0) (- . 1) (* . 2) . 3))))
(tn "assoc improper lists: numbers")
(assert-error  (tn) (lambda () (assoc 0 '0)))
(assert-equal? (tn) '(1 . b)   (assoc 1 '((0 . a) (1 . b) (3 . c) . d)))
(assert-error  (tn) (lambda () (assoc 4 '((0 . a) (1 . b) (3 . c) . d))))
(tn "assoc improper lists: chars")
(assert-error  (tn) (lambda () (assoc #\a #\a)))
(assert-equal? (tn) '(#\b . 1) (assoc #\b
                                      '((#\a . 0) (#\b . 1) (#\c . 2) . 3)))
(assert-equal? (tn) '(#\b . 1) (assoc (char #\b)
                                      '((#\a . 0) (#\b . 1) (#\c . 2) . 3)))
(assert-error  (tn) (lambda () (assoc #\d
                                      '((#\a . 0) (#\b . 1) (#\c . 2) . 3))))
(tn "assoc improper lists: strings")
(assert-error  (tn) (lambda () (assoc "a" "a")))
(assert-equal? (tn) '("b" . 1) (assoc "b"
                                      '(("a" . 0) ("b" . 1) ("c" . 2) . 3)))
(assert-equal? (tn) '("b" . 1) (assoc (string #\b)
                                      '(("a" . 0) ("b" . 1) ("c" . 2) . 3)))
(assert-error  (tn) (lambda () (assoc "d"
                                      '(("a" . 0) ("b" . 1) ("c" . 2) . 3))))
(tn "assoc improper lists: lists")
(assert-error  (tn) (lambda () (assoc ("a") ("a"))))
(assert-equal? (tn)
               '(("b") . 1)
               (assoc '("b") '((("a") . 0) (("b") . 1) (("c") . 2) . 3)))
(assert-equal? (tn)
               '(("b") . 1)
               (assoc (list (string #\b))
                      '((("a") . 0) (("b") . 1) (("c") . 2) . 3)))
(assert-error  (tn)
               (lambda ()
                 (assoc ("d") '((("a") . 0) (("b") . 1) (("c") . 2) . 3))))


(total-report)
