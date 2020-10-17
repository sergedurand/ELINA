/***************************************************************************************************
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of
 *       conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TOR (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************************************/
/*! \file
    \brief Implementation of a CTA-wide semaphore for inter-CTA synchronization.
*/

#pragma once

#include "cutlass/cutlass.h"

#include "cutlass/aligned_buffer.h"
#include "cutlass/array.h"

#include "cutlass/numeric_types.h"
#include "cutlass/matrix_shape.h"

#include "cutlass/gemm/gemm.h"

/////////////////////////////////////////////////////////////////////////////////////////////////

namespace cutlass {

/////////////////////////////////////////////////////////////////////////////////////////////////

/// CTA-wide semaphore for inter-CTA synchronization.
class Semaphore { 
public:

  int *lock;
  bool wait_thread;
  int state;

public:

  /// Implements a semaphore to wait for a flag to reach a given value
  CUTLASS_HOST_DEVICE
  Semaphore(int *lock_, int thread_id): 
    lock(lock_), 
    wait_thread(thread_id < 0 || thread_id == 0),
    state(-1) {

  }

  /// Permit fetching the synchronization mechanism early
  CUTLASS_DEVICE
  void fetch() {
    if (wait_thread) {
      asm volatile ("ld.global.cg.b32 %0, [%1];\n" : "=r"(state) : "l"(lock));  
    }
  }

  /// Gets the internal state
  CUTLASS_DEVICE
  int get_state() const {
    return state;
  }

  /// Waits until the semaphore is equal to the given value
  CUTLASS_DEVICE
  void wait(int status = 0) {

    while( __syncthreads_and(state != status) ) {
      fetch();
    }

    __syncthreads();
  }

  /// Updates the lock with the given result
  CUTLASS_DEVICE
  void release(int status = 0) {
    __syncthreads();

    if (wait_thread) {
      asm volatile ("st.global.cg.b32 [%0], %1;\n" : : "l"(lock), "r"(status));
    }
  }
};

/////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace cutlass

/////////////////////////////////////////////////////////////////////////////////////////////////
