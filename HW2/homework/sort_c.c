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


#define SORT_CUTOFF_1 80
#define SORT_CUTOFF_2 800
#define SORT_CUTOFF_3 100
#define SORT_CUTOFF_4 160

// Function prototypes
static void merge_c(data_t* A, int p, int q, int r);
static void copy_c(data_t* source, data_t* dest, int n);
void sort_c_0(data_t* A, int p, int r);

/* Different cutoff values */
void sort_c_1(data_t* A, int p, int r);
void sort_c_2(data_t* A, int p, int r);
void sort_c_3(data_t* A, int p, int r);
void sort_c_4(data_t* A, int p, int r);

/* Different sorting algorithms */
void sort_c_insert(data_t* A, int p, int r);
void sort_c_select(data_t* A, int p, int r);
void sort_c_bubble(data_t* A, int p, int r);
void sort_c_shell(data_t* A, int p, int r);

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

static void inline selection_sort(int arr[], int n) {
    int i, j, min_idx, temp;
    for (i = 0; i < n - 1; i++) {
        min_idx = i;
        for (j = i + 1; j < n; j++) {
            if (arr[j] < arr[min_idx])
                min_idx = j;
        }
        // 交换
        temp = arr[min_idx];
        arr[min_idx] = arr[i];
        arr[i] = temp;
    }
}

static void inline bubble_sort(int arr[], int n) {
    int i, j, temp;
    for (i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

static void inline shell_sort(int arr[], int n) {
    int gap, i, j, temp;
    for (gap = n / 2; gap > 0; gap /= 2) {
        for (i = gap; i < n; i++) {
            temp = arr[i];
            for (j = i; j >= gap && arr[j - gap] > temp; j -= gap) {
                arr[j] = arr[j - gap];
            }
            arr[j] = temp;
        }
    }
}

void inline sort_c_insert(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_1) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_c_insert(A, p, q);
    sort_c_insert(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

void inline sort_c_select(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_1) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_c_select(A, p, q);
    sort_c_select(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

void inline sort_c_bubble(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_1) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_c_bubble(A, p, q);
    sort_c_bubble(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

void inline sort_c_shell(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_1) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_c_shell(A, p, q);
    sort_c_shell(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

// A basic merge sort routine that sorts the subarray A[p..r]
void inline sort_c_0(data_t* A, int p, int r) {
  assert(A);
  if (p < r) {
    int q = (p + r) / 2;
    sort_c_0(A, p, q);
    sort_c_0(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

void inline sort_c_1(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_1) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_c_1(A, p, q);
    sort_c_1(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

void inline sort_c_2(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_2) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_c_2(A, p, q);
    sort_c_2(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

void inline sort_c_3(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_3) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_c_3(A, p, q);
    sort_c_3(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

void inline sort_c_4(data_t* A, int p, int r) {
  assert(A);

  if ((r - p) < SORT_CUTOFF_4) {
    isort(A + p, A + r);
  } else {
    int q = (p + r) / 2;
    sort_c_4(A, p, q);
    sort_c_4(A, q + 1, r);
    merge_c(A, p, q, r);
  }
}

// A merge routine. Merges the sub-arrays A [p..q] and A [q + 1..r].
// Uses two arrays 'left' and 'right' in the merge operation.
static void merge_c(data_t* A, int p, int q, int r) {
  assert(A);
  assert(p <= q);
  assert((q + 1) <= r);
  int n1 = q - p + 1;
  int n2 = r - q;

  data_t* left = 0, * right = 0;
  mem_alloc(&left, n1 + 1);
  mem_alloc(&right, n2 + 1);
  if (left == NULL || right == NULL) {
    mem_free(&left);
    mem_free(&right);
    return;
  }

  copy_c(&(A[p]), left, n1);
  copy_c(&(A[q + 1]), right, n2);
  left[n1] = UINT_MAX;
  right[n2] = UINT_MAX;

  data_t *a_ptr = A + p;
  data_t *left_ptr = left;
  data_t *right_ptr = right;

  for (int k = p; k <= r; k++) {
    if (*left_ptr <= *right_ptr) {
      *a_ptr = *left_ptr;
      left_ptr++;
    } else {
      *a_ptr = *right_ptr;
      right_ptr++;
    }
    a_ptr++;
  }
  mem_free(&left);
  mem_free(&right);
}

static __attribute__((always_inline)) void copy_c(data_t* source, data_t* dest, int n) {
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
