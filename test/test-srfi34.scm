;;  FileName : test-srfi34.scm
;;  About    : unit test for SRFI-34
;;
;;  Copyright (C) 2005      by Kazuki Ohta (mover@hct.zaq.ne.jp)
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

(cond-expand
 (sigscheme
  (use srfi-34))
 (else #t))

;; with-exception-handler

;; Although the behavior when a handler returned is not specified in SRFI-34,
;; SigScheme should produce an error to prevent being misused by users.
(display "with-exception-handler #1")
(newline)
(assert-error "with-exception-handler #1"
              (lambda ()
                (with-exception-handler
                    (lambda (x)
                      'a-handler-must-not-return)
                  (lambda ()
                    (+ 1 (raise 'an-error))))))

(display "with-exception-handler #2")
(newline)
(assert-error "with-exception-handler #3"
              (lambda ()
                (with-exception-handler
                    (lambda (x)
                      (assert-equal? "with-exception-handler #2" 'an-error x)
                      (display "with-exception-handler #3")
                      (newline)
                      'a-handler-must-not-return)
                  (lambda ()
                    (+ 1 (raise 'an-error))))))

(display "with-exception-handler #4")
(newline)
(assert-equal? "with-exception-handler #4"
               6
	       (with-exception-handler
                   (lambda (x)
                     'not-reaches-here)
                 (lambda ()
                   (+ 1 2 3))))

(display "with-exception-handler #5")
(newline)
(assert-equal? "with-exception-handler #5"
               'success
	       (with-exception-handler
                   (lambda (x)
                     'not-reaches-here)
                 (lambda ()
                   'success)))


;; guard
(display "guard #1")
(newline)
(assert-equal? "guard #1"
               'exception
	       (guard (condition
		       (else
                        (display "guard #2")
                        (newline)
			(assert-equal? "guard #2" 'an-error condition)
			'exception))
                 (+ 1 (raise 'an-error))))

(display "guard #3")
(newline)
(assert-equal? "guard #3"
               3
	       (guard (condition
		       (else
			'exception))
                 (+ 1 2)))

(display "guard #4")
(newline)
(assert-equal? "guard #4"
               'success
	       (guard (condition
		       (else
			'exception))
                 'success))

(display "guard #5")
(newline)
(assert-equal? "guard #5"
               'exception
	       (guard (condition
		       (else
			'exception))
                 (+ 1 (raise 'error))))

(display "guard #6")
(newline)
(assert-equal? "guard #6"
               42
               (guard (condition
                       ((assq 'a condition) => cdr)
                       ((assq 'b condition))
                       (else
                        (display condition)
                        (newline)))
                 (raise (list (cons 'a 42)))))

(display "guard #7")
(newline)
(assert-equal? "guard #7"
               '(b . 23)
               (guard (condition
                       ((assq 'a condition) => cdr)
                       ((assq 'b condition))
                       (else
                        (display condition)
                        (newline)))
                 (raise (list (cons 'b 23)))))


;; mixed use of with-exception-handler and guard
(display "mixed exception handling #1")
(newline)
(assert-equal? "mixed exception handling #1"
               'guard-ret
	       (with-exception-handler (lambda (x)
					 (k 'with-exception-ret))
                 (lambda ()
                   (guard (condition
                           (else
                            'guard-ret))
                     (raise 1)))))

(display "mixed exception handling #2")
(newline)
(assert-equal? "mixed exception handling #2"
               'with-exception-ret
	       (with-exception-handler (lambda (x)
					 'with-exception-ret)
                 (lambda ()
                   (guard (condition
                           ((negative? condition)
                            'guard-ret))
                     (raise 1)))))

(display "mixed exception handling #3")
(newline)
(assert-equal? "mixed exception handling #3"
               'positive
               (call-with-current-continuation
                (lambda (k)
                  (with-exception-handler (lambda (x)
                                            (k 'zero))
                    (lambda ()
                      (guard (condition
                              ((positive? condition) 'positive)
                              ((negative? condition) 'negative))
                        (raise 1)))))))

(display "mixed exception handling #4")
(newline)
(assert-equal? "mixed exception handling #4"
               'negative
               (call-with-current-continuation
                (lambda (k)
                  (with-exception-handler (lambda (x)
                                            (k 'zero))
                    (lambda ()
                      (guard (condition
                              ((positive? condition) 'positive)
                              ((negative? condition) 'negative))
                        (raise -1)))))))

(display "mixed exception handling #5")
(newline)
(assert-equal? "mixed exception handling #5"
               'zero
               (call-with-current-continuation
                (lambda (k)
                  (with-exception-handler (lambda (x)
                                            (k 'zero))
                    (lambda ()
                      (guard (condition
                              ((positive? condition) 'positive)
                              ((negative? condition) 'negative))
                        (raise 0)))))))

(total-report)
