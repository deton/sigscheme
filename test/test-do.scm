;;  Filename : test-do.scm
;;  About    : unit test for R5RS 'do' syntax
;;
;;  Copyright (C) 2005-2006 Kazuki Ohta <mover AT hct.zaq.ne.jp>
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

(load "./test/unittest.scm")

(define *test-track-progress* #f)
(define tn test-name)


;;
;; do
;;
(tn "do invalid form")
(assert-error  (tn) (lambda ()
                      (do)))
(assert-error  (tn) (lambda ()
                      (do v)))
(assert-error  (tn) (lambda ()
                      (do (v 1))))
(assert-error  (tn) (lambda ()
                      (do ((v 1))
                          )))
(assert-error  (tn) (lambda ()
                      (do ((v))
                          'eval)))
(assert-error  (tn) (lambda ()
                      (do ((v 1))
                          'unknow-value)))
(assert-error  (tn) (lambda ()
                      (do ((v 1 2 'excessive))
                          'eval)))
(tn "do invalid form: no test")
(assert-error  (tn) (lambda ()
                      (do ((v 1))
                          ()
                        'eval)))
(tn "do invalid form: non-list test form")
(assert-error  (tn) (lambda ()
                      (do ((v 1))
                          'test
                        'eval)))
(assert-error  (tn) (lambda ()
                      (do ((v 1))
                          1
                        'eval)))
(tn "do invalid form: non-list bindings form")
(assert-error  (tn) (lambda ()
                      (do 'bindings
                          (#t #t)
                        'eval)))
(assert-error  (tn) (lambda ()
                      (do 1
                          (#t #t)
                        'eval)))
(tn "do invalid form: non-symbol variable name")
(assert-error  (tn) (lambda ()
                      (do ((1 1))
                          (#t #t)
                        #t)))
(assert-error  (tn) (lambda ()
                      (do ((#t 1))
                          (#t #t)
                        #t)))
(assert-error  (tn) (lambda ()
                      (do (("a" 1))
                          (#t #t)
                        #t)))
(tn "do invalid form: duplicate variable name")
(assert-error  (tn) (lambda ()
                      (do ((v 1)
                           (v 2))
                          (#t #t)
                        #t)))
(assert-error  (tn) (lambda ()
                      (do ((v 1)
                           (w 0)
                           (v 2))
                          (#t #t)
                        #t)))
(tn "do invalid form: improper binding")
(assert-error  (tn) (lambda ()
                      (do ((v . 1))
                          (#t #t)
                        #t)))
(assert-error  (tn) (lambda ()
                      (do ((v  1 . v))
                          (#t #t)
                        #t)))
(assert-error  (tn) (lambda ()
                      (do ((v  1) . 1)
                          (#t #t)
                        #t)))
(tn "do invalid form: improper exps")
(assert-error  (tn) (lambda ()
                      (do ((v  1))
                          (#t . #t)
                        #t)))
(assert-error  (tn) (lambda ()
                      (do ((v  1))
                          (#t #t . #t)
                        #t)))
(tn "do invalid form: improper commands")
(assert-error  (tn) (lambda ()
                      (do ((v  1))
                          (#t #t)
                        #t . #t)))
(assert-error  (tn) (lambda ()
                      (do ((v  1 (+ v 1)))
                          ((= v 2) #t)
                        #t . #t)))
(tn "do invalid form: 'define' at <init>")
;; Since <init>s are evaled in toplevel env, these tests cause error only when
;; SCM_STRICT_TOPLEVEL_DEFINITIONS.
(if (provided? "strict-toplevel-definitions")
    (begin
      (assert-error  (tn)
                     (lambda ()
                       (eval '(do ((i (define var1 1)))
                                  (#t #t)
                                )
                             (interaction-environment))))
      (assert-error  (tn)
                     (lambda ()
                       (eval '(do ((i (begin)))
                                  (#t #t)
                                )
                             (interaction-environment))))
      (assert-error  (tn)
                     (lambda ()
                       (eval '(do ((i (begin (define var1 1))))
                                  (#t #t)
                                )
                             (interaction-environment))))
      (assert-error  (tn)
                     (lambda ()
                       (eval '(do ((i (begin (define var1 1) 1)))
                                  (#t #t)
                                )
                             (interaction-environment))))
      (assert-error  (tn)
                     (lambda ()
                       (eval '(do ((i (begin 1 (define var1 1))))
                                  (#t #t)
                                )
                             (interaction-environment))))))
(tn "do invalid form: 'define' at <step>")
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0 (define var1 1)))
                            (#f #t)
                          )
                       (interaction-environment))))
(if (provided? "strict-toplevel-definitions")
    (assert-error  (tn)
                   (lambda ()
                     (eval '(do ((i 0 (begin)))
                                (#f #t)
                              )
                           (interaction-environment)))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0 (begin (define var1 1))))
                            (#f #t)
                          )
                       (interaction-environment))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0 (begin (define var1 1) 1)))
                            (#f #t)
                          )
                       (interaction-environment))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0 (begin 1 (define var1 1))))
                            (#f #t)
                          )
                       (interaction-environment))))
(tn "do invalid form: 'define' at <expression>s")
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0))
                            (#t (define var1 1))
                          )
                       (interaction-environment))))
(if (provided? "strict-toplevel-definitions")
    (assert-error  (tn)
                   (lambda ()
                     (eval '(do ((i 0))
                                (#t (begin))
                              )
                           (interaction-environment)))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0))
                            (#t (begin (define var1 1)))
                          )
                       (interaction-environment))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0))
                            (#t (begin (define var1 1) 1))
                          )
                       (interaction-environment))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0))
                            (#t (begin 1 (define var1 1)))
                          )
                       (interaction-environment))))
(tn "do invalid form: 'define' at <command>s")
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0 (+ i 1)))
                            ((= i 1) #t)
                          (define var1 1))
                       (interaction-environment))))
(if (provided? "strict-toplevel-definitions")
    (assert-error  (tn)
                   (lambda ()
                     (eval '(do ((i 0 (+ i 1)))
                                ((= i 1) #t)
                              (begin))
                           (interaction-environment)))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0 (+ i 1)))
                            ((= i 1) #t)
                          (begin
                            (define var1 1)))
                       (interaction-environment))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0 (+ i 1)))
                            ((= i 1) #t)
                          (begin
                            (define var1 1)
                            1))
                       (interaction-environment))))
(assert-error  (tn)
               (lambda ()
                 (eval '(do ((i 0 (+ i 1)))
                            ((= i 1) #t)
                          (begin
                            1
                            (define var1 1)))
                       (interaction-environment))))

(tn "do invalid form: binding syntactic keywords on <init>")
(assert-error  (tn)
               (lambda ()
                 (do ((syn define))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn if))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn and))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn cond))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn begin))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn do))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn delay))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn let*))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn else))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn =>))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn quote))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn quasiquote))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn unquote))
                     (#t #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn unquote-splicing))
                     (#t #t)
                   #t)))

(tn "do invalid form: binding syntactic keywords on <step>")
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t define))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t if))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t and))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t cond))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t begin))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t do))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t delay))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t let*))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t else))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t =>))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t quote))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t quasiquote))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t unquote))
                     (#f #t)
                   #t)))
(assert-error  (tn)
               (lambda ()
                 (do ((syn #t unquote-splicing))
                     (#f #t)
                   #t)))

(tn "do valid form: no bindings")
(assert-true   (tn) (lambda ()
                      (do ()
                          (#t #t)
                        'foo)))
(assert-true   (tn) (lambda ()
                      (do ()
                          (#t)
                        'foo)))
(assert-true   (tn) (lambda ()
                      (do ()
                          (#t #t)
                        )))
(assert-true   (tn) (lambda ()
                      (do ()
                          (#t)
                        )))
(tn "do valid form: no commands")
(assert-true   (tn) (lambda ()
                      (do ((v 1))
                          (#t #t)
                        )))
(assert-true   (tn) (lambda ()
                      (do ((v 1))
                          (#t)
                        )))
(tn "do valid form: no exps")
(if (provided? "sigscheme")
    (assert-equal? (tn)
                   (undef)
                   (do ((v 1))
                       (#t)
                     'foo)))

(tn "do inter-iteration variable isolation")
(assert-equal? (tn)
               '(2 1 0)
               (do ((v '() (cons i v))
                    (i 0 (+ i 1)))
                   ((= i 3) v)
                 ))
(assert-equal? (tn)
               '(2 1 0)
               (do ((i 0 (+ i 1))
                    (v '() (cons i v)))
                   ((= i 3) v)
                 ))

(tn "do initialize-time variable isolation")  
(assert-error (tn) (lambda () (do ((v 1)
                                   (w v))
                                  (#t #t)
                                )))
(assert-error (tn) (lambda () (do ((w v)
                                   (v 1))
                                  (#t #t)
                                )))

(tn "do exp is evaluated exactly once")
(assert-equal? (tn)
               '(+ v w)
               (do ((v 1)
                    (w 2))
                   (#t '(+ v w))
                 ))

(tn "do iteration count")
(assert-equal? (tn)
               0
               (do ((i 0 (+ i 1))
                    (evaled 0))
                   (#t evaled)
                 (set! evaled (+ evaled 1))))
(assert-equal? (tn)
               0
               (do ((i 0 (+ i 1))
                    (evaled 0))
                   ((= i 0) evaled)
                 (set! evaled (+ evaled 1))))
(assert-equal? (tn)
               1
               (do ((i 0 (+ i 1))
                    (evaled 0))
                   ((= i 1) evaled)
                 (set! evaled (+ evaled 1))))
(assert-equal? (tn)
               2
               (do ((i 0 (+ i 1))
                    (evaled 0))
                   ((= i 2) evaled)
                 (set! evaled (+ evaled 1))))

(tn "do variable update")
(assert-equal? (tn)
               10
               (do ((v 1)
                    (w 2))
                   (#t (set! v (+ v 1))
                       (set! w (+ w v))
                       (set! v (+ v w))
                       (+ w v))
                 ))
(assert-equal? (tn)
               16
               (do ((i 0 (+ i 1))
                    (v 1)
                    (w 2))
                   ((= i 1)
                    (set! v (+ v 1))
                    (set! w (+ w v))
                    (set! v (+ v w))
                    (+ w v))
                 (set! v 3)))
(assert-equal? (tn)
               20
               (do ((i 0 (+ i 1))
                    (v 1)
                    (w 2))
                   ((= i 1)
                    (set! v (+ v 1))
                    (set! w (+ w v))
                    (set! v (+ v w))
                    (+ w v))
                 (set! v 3)
                 (set! w 4)))

(tn "do per-iteration env isolation")
(assert-equal? (tn)
               '(4 3 2 1 0)
               (do ((i 0 (+ i 1))
                    (procs '() (cons (lambda () i) procs)))
                   ((= i 5) (map (lambda (p) (p)) procs))
                 ))
(assert-equal? (tn)
               '(8 6 4 2 0)
               (do ((i 0 (+ i 1))
                    (procs '() (cons (lambda ()
                                       (set! i (* i 2)) i)
                                     procs)))
                   ((= i 5) (map (lambda (p) (p)) procs))
                 ))
(assert-equal? (tn)
               '(4 3 2 1 0)
               (do ((i 0 (+ i 1))
                    (procs '() (cons (lambda () i) procs)))
                   ((= i 5)
                    (set! i 1024)
                    (map (lambda (p) (p)) procs))
                 ))

(assert-equal? "do test1" '#(0 1 2 3 4) (do ((vec (make-vector 5))
					     (i 0 (+ i 1)))
					    ((= i 5) vec)
					  (vector-set! vec i i)))
(assert-equal? "do test2" 25 (let ((x '(1 3 5 7 9)))
			       (do ((x x (cdr x))
				    (sum 0 (+ sum (car x))))
				   ((null? x) sum))))

(define (expt-do x n)
  (do ((i 0 (+ i 1))
       (y 1))
      ((= i n) y)
    (set! y (* x y))))
(assert-equal? "do test3" 1024 (expt-do 2 10))

(define (nreverse rev-it)
  (do ((reved '() rev-it)
       (rev-cdr (cdr rev-it) (cdr rev-cdr))
       (rev-it rev-it rev-cdr))
      ((begin
	 (set-cdr! rev-it reved)
	 (null? rev-cdr))
       rev-it)))
(assert-equal? "do test4" '(c b a) (nreverse (list 'a 'b 'c)))
(assert-equal? "do test5"
               '((5 6) (3 4) (1 2))
               (nreverse (list '(1 2) '(3 4) '(5 6))))

;; scm_s_do() has been changed as specified in R5RS. -- YamaKen 2006-01-11
;; R5RS: If no <expression>s are present, then the value of the `do' expression
;; is unspecified.
;;(assert-equal? "do test6" 1  (do ((a 1)) (a) 'some))
;;(assert-equal? "do test7" #t (do ((a 1)) (#t) 'some))
(if (provided? "sigscheme")
    (begin
      (assert-equal? "do test6" (undef) (do ((a 1)) (a) 'some))
      (assert-equal? "do test7" (undef) (do ((a 1)) (#t) 'some))))
;; (do ((a 1)) 'eval) => (do ((a 1)) (quote eval))
(assert-equal? "do test8" eval (do ((a 1)) 'eval))


(total-report)
