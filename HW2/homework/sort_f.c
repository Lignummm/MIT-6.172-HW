 /**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/


#include "./util.h"


#define SORT_CUTOFF_80 80

static data_t *left = NULL;

// Function prototypes
static void merge_f(data_t* A, int p, int q, int r);
static void copy_f(data_t* source, data_t* dest, int n);
void inner_sort_f(data_t* A, int p, int r);

// Insertion sort, sorting the array between begin and end, inclusive
static void inline isort(data_t* begin, data_t* end) {
  data_t* cur = begin + 1;
  while (cur <= end) {
    data_t val = *cur;
    data_t* index = cur - 1;

    while (index >= begin && *index > val) {
      *(index + 1) = *index;
      index--;
    }

    *(index + 1) = val;
    cur++;
  }
}

void sort_f(data_t* A, int p, int r) {
  mem_alloc(&left, (r - p + 2) / 2);
  if (left == NULL) {
    return;
  }

  inner_sort_f(A,p,r);

  mem_free(&left);
}

void inline inner_sort_f(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_80) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    inner_sort_f(A, p, q);
    inner_sort_f(A, q + 1, r);
    merge_f(A, p, q, r);
  }
}

// A merge routine. Merges the sub-arrays A [p..q] and A [q + 1..r].
static void merge_f(data_t* A, int p, int q, int r) {
  assert(A);
  assert(p <= q);
  assert((q + 1) <= r);
  int n1 = q - p + 1;

  copy_f(&(A[p]), left, n1);

  data_t* lp = left;
  data_t* le = left + n1;
  data_t* rp = A + q + 1;
  data_t* re = A + r + 1;
  data_t* dst = A + p;

  while (lp < le && rp < re) {
    if (*lp <= *rp) {
      *dst++ = *lp++;
    } else {
      *dst++ = *rp++;
    }
  }

  while (lp < le) {
    *dst++ = *lp++;
  }
}

static __attribute__((always_inline)) void copy_f(data_t* source, data_t* dest, int n) {
  assert(dest);
  assert(source);
  data_t* source_ptr = source;
  data_t* dest_ptr = dest;

  for (int i = 0 ; i < n ; i++) {
    *dest_ptr = *source_ptr;
    source_ptr++;
    dest_ptr++;
  }
}
