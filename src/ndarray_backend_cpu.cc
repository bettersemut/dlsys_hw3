#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <stdio.h>

namespace needle {
namespace cpu {

#define ALIGNMENT 256
#define TILE 8
typedef float scalar_t;
const size_t ELEM_SIZE = sizeof(scalar_t);


/**
 * This is a utility structure for maintaining an array aligned to ALIGNMENT boundaries in
 * memory.  This alignment should be at least TILE * ELEM_SIZE, though we make it even larger
 * here by default.
 */
struct AlignedArray {
  AlignedArray(const size_t size) {
    int ret = posix_memalign((void**)&ptr, ALIGNMENT, size * ELEM_SIZE);
    if (ret != 0) throw std::bad_alloc();
    this->size = size;
  }
  ~AlignedArray() { free(ptr); }
  size_t ptr_as_int() {return (size_t)ptr; }
  scalar_t* ptr;
  size_t size;
};



void Fill(AlignedArray* out, scalar_t val) {
  /**
   * Fill the values of an aligned array with val
   */
  for (int i = 0; i < out->size; i++) {
    out->ptr[i] = val;
  }
}


void Compact(const AlignedArray& a, AlignedArray* out, std::vector<uint32_t> shape,
             std::vector<uint32_t> strides, size_t offset) {
  /**
   * Compact an array in memory
   *
   * Args:
   *   a: non-compact representation of the array, given as input
   *   out: compact version of the array to be written
   *   shape: shapes of each dimension for a and out
   *   strides: strides of the *a* array (not out, which has compact strides)
   *   offset: offset of the *a* array (not out, which has zero offset, being compact)
   *
   * Returns:
   *  void (you need to modify out directly, rather than returning anything; this is true for all the
   *  function will implement here, so we won't repeat this note.)
   */
  /// BEGIN YOUR SOLUTION
  assert (shape.size() == strides.size());
  size_t ndim = shape.size();
  std::vector<uint32_t> index(ndim, 0);
  size_t cnt = 0;
  // printf("size %d\n", out->size);
  // for(size_t i=0;i<ndim;i++) {
  //   printf("input shape %d, stride %d\n", shape.at(i), strides.at(i));
  // }
  while(true) {
    // 操作一次
    size_t cur_offset = offset;
    // printf("------------------------------");
    // printf("cnt: %u\n", cnt);
    for(size_t i=0;i<ndim;i++) {
      cur_offset += index.at(i) * strides.at(i);
      // printf("dim %d, index %d, stride %d\n", i, index.at(i), strides.at(i));
    }
    // printf("offset %d\n", cur_offset);
    out->ptr[cnt] = a.ptr[cur_offset];
    // 更新cnt以及index
    cnt++;
    if (cnt >= out->size) {
      break;
    }
    for(size_t j=ndim-1;j>=0;j--) {
      if (index.at(j) != shape.at(j) - 1) {
        break;
      } else {
        index.pop_back();
      }
    }
    uint32_t last_value = index.back();
    index.pop_back();
    index.push_back(last_value + 1);
    while (index.size() < ndim) {
      index.push_back(0);
    }
    // for(size_t i=0;i<index.size();i++) {
    //   printf("dim %d, index %d\n", i, index.at(i));
    // }
  }
  // printf("compact process end\n");
}

void EwiseSetitem(const AlignedArray& a, AlignedArray* out, std::vector<uint32_t> shape,
                  std::vector<uint32_t> strides, size_t offset) {
  /**
   * Set items in a (non-compact) array
   *
   * Args:
   *   a: _compact_ array whose items will be written to out
   *   out: non-compact array whose items are to be written
   *   shape: shapes of each dimension for a and out
   *   strides: strides of the *out* array (not a, which has compact strides)
   *   offset: offset of the *out* array (not a, which has zero offset, being compact)
   */
  size_t ndim = shape.size();
  std::vector<uint32_t> index(ndim, 0);
  size_t cnt = 0;
  // printf("size %d\n", out->size);
  // for(size_t i=0;i<ndim;i++) {
  //   printf("input shape %d, stride %d\n", shape.at(i), strides.at(i));
  // }
  while(true) {
    // 操作一次
    size_t cur_offset = offset;
    // printf("------------------------------");
    // printf("cnt: %u\n", cnt);
    for(size_t i=0;i<ndim;i++) {
      cur_offset += index.at(i) * strides.at(i);
      // printf("dim %d, index %d, stride %d\n", i, index.at(i), strides.at(i));
    }
    // printf("offset %d\n", cur_offset);
    out->ptr[cur_offset] = a.ptr[cnt];
    // 更新cnt以及index
    cnt++;
    if (cnt >= a.size) {
      break;
    }
    for(size_t j=ndim-1;j>=0;j--) {
      if (index.at(j) != shape.at(j) - 1) {
        break;
      } else {
        index.pop_back();
      }
    }
    uint32_t last_value = index.back();
    index.pop_back();
    index.push_back(last_value + 1);
    while (index.size() < ndim) {
      index.push_back(0);
    }
    // for(size_t i=0;i<index.size();i++) {
    //   printf("dim %d, index %d\n", i, index.at(i));
    // }
  }
}

void ScalarSetitem(const size_t size, scalar_t val, AlignedArray* out, std::vector<uint32_t> shape,
                   std::vector<uint32_t> strides, size_t offset) {
  /**
   * Set items is a (non-compact) array
   *
   * Args:
   *   size: number of elements to write in out array (note that this will note be the same as
   *         out.size, because out is a non-compact subset array);  it _will_ be the same as the
   *         product of items in shape, but convenient to just pass it here.
   *   val: scalar value to write to
   *   out: non-compact array whose items are to be written
   *   shape: shapes of each dimension of out
   *   strides: strides of the out array
   *   offset: offset of the out array
   */
  size_t ndim = shape.size();
  std::vector<uint32_t> index(ndim, 0);
  size_t cnt = 0;
  // printf("size %d\n", out->size);
  // for(size_t i=0;i<ndim;i++) {
  //   printf("input shape %d, stride %d\n", shape.at(i), strides.at(i));
  // }
  while(true) {
    // 操作一次
    size_t cur_offset = offset;
    // printf("------------------------------");
    // printf("cnt: %u\n", cnt);
    for(size_t i=0;i<ndim;i++) {
      cur_offset += index.at(i) * strides.at(i);
      // printf("dim %d, index %d, stride %d\n", i, index.at(i), strides.at(i));
    }
    // printf("offset %d\n", cur_offset);
    out->ptr[cur_offset] = val;
    // 更新cnt以及index
    cnt++;
    if (cnt >= size) {
      break;
    }
    for(size_t j=ndim-1;j>=0;j--) {
      if (index.at(j) != shape.at(j) - 1) {
        break;
      } else {
        index.pop_back();
      }
    }
    uint32_t last_value = index.back();
    index.pop_back();
    index.push_back(last_value + 1);
    while (index.size() < ndim) {
      index.push_back(0);
    }
    // for(size_t i=0;i<index.size();i++) {
    //   printf("dim %d, index %d\n", i, index.at(i));
    // }
  }
}

void EwiseAdd(const AlignedArray& a, const AlignedArray& b, AlignedArray* out) {
  /**
   * Set entries in out to be the sum of correspondings entires in a and b.
   */
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] + b.ptr[i];
  }
}

void ScalarAdd(const AlignedArray& a, scalar_t val, AlignedArray* out) {
  /**
   * Set entries in out to be the sum of corresponding entry in a plus the scalar val.
   */
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] + val;
  }
}


/**
 * In the code the follows, use the above template to create analogous element-wise
 * and and scalar operators for the following functions.  See the numpy backend for
 * examples of how they should work.
 *   - EwiseMul, ScalarMul
 *   - EwiseDiv, ScalarDiv
 *   - ScalarPower
 *   - EwiseMaximum, ScalarMaximum
 *   - EwiseEq, ScalarEq
 *   - EwiseGe, ScalarGe
 *   - EwiseLog
 *   - EwiseExp
 *   - EwiseTanh
 *
 * If you implement all these naively, there will be a lot of repeated code, so
 * you are welcome (but not required), to use macros or templates to define these
 * functions (however you want to do so, as long as the functions match the proper)
 * signatures above.
 */
 
void EwiseMul(const AlignedArray& a, const AlignedArray& b, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] * b.ptr[i];
  }
}

void EwiseDiv(const AlignedArray& a, const AlignedArray& b, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] / b.ptr[i];
  }
}

void EwiseMaximum(const AlignedArray& a, const AlignedArray& b, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] > b.ptr[i] ? a.ptr[i] : b.ptr[i];
  }
}

void EwiseEq(const AlignedArray& a, const AlignedArray& b, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] == b.ptr[i] ? 1.0 : 0.0;
  }
}

void EwiseGe(const AlignedArray& a, const AlignedArray& b, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] > b.ptr[i] ? 1.0 : 0.0;
  }
}

void ScalarMul(const AlignedArray& a, scalar_t val, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] * val;
  }
}


void ScalarDiv(const AlignedArray& a, scalar_t val, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] / val;
  }
}


void ScalarPower(const AlignedArray& a, scalar_t val, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = pow(a.ptr[i], val);
  }
}


void ScalarMaximum(const AlignedArray& a, scalar_t val, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] > val ? a.ptr[i] : val;
  }
}


void ScalarEq(const AlignedArray& a, scalar_t val, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] == val ? 1.0 : 0.0;
  }
}


void ScalarGe(const AlignedArray& a, scalar_t val, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = a.ptr[i] >= val ? 1.0 : 0.0;
  }
}

void EwiseLog(const AlignedArray& a, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = log(a.ptr[i]);
  }
}


void EwiseExp(const AlignedArray& a, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = exp(a.ptr[i]);
  }
}


void EwiseAbs(const AlignedArray& a, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = abs(a.ptr[i]);
  }
}


void EwiseTanh(const AlignedArray& a, AlignedArray* out) {
  for (size_t i = 0; i < a.size; i++) {
    out->ptr[i] = tanh(a.ptr[i]);
  }
}


void Matmul(const AlignedArray& a, const AlignedArray& b, AlignedArray* out, uint32_t m, uint32_t n,
            uint32_t p) {
  /**
   * Multiply two (compact) matrices into an output (also compact) matrix.  For this implementation
   * you can use the "naive" three-loop algorithm.
   *
   * Args:
   *   a: compact 2D array of size m x n
   *   b: compact 2D array of size n x p
   *   out: compact 2D array of size m x p to write the output to
   *   m: rows of a / out
   *   n: columns of a / rows of b
   *   p: columns of b / out
   */
  // printf("-----------------------------------\n");
  // printf("m=%d,n=%d,p=%d\n", m, n, p);
  for(size_t i=0;i<m;i++) {
    for(size_t k=0;k<p;k++){
      scalar_t tmp = 0.0;
      for(size_t j=0;j<n;j++){
        tmp += a.ptr[i * n + j] * b.ptr[j * p + k];
      }
      out->ptr[i * p + k] = tmp;
      // printf("i=%d, k=%d, index=%d, value=%f\n",i, k, i * p + k, tmp);
    }
  }
}

inline void AlignedDot(const float* __restrict__ a,
                       const float* __restrict__ b,
                       float* __restrict__ out) {

  /**
   * Multiply together two TILE x TILE matrices, and _add _the result to out (it is important to add
   * the result to the existing out, which you should not set to zero beforehand).  We are including
   * the compiler flags here that enable the compile to properly use vector operators to implement
   * this function.  Specifically, the __restrict__ keyword indicates to the compile that a, b, and
   * out don't have any overlapping memory (which is necessary in order for vector operations to be
   * equivalent to their non-vectorized counterparts (imagine what could happen otherwise if a, b,
   * and out had overlapping memory).  Similarly the __builtin_assume_aligned keyword tells the
   * compiler that the input array will be aligned to the appropriate blocks in memory, which also
   * helps the compiler vectorize the code.
   *
   * Args:
   *   a: compact 2D array of size TILE x TILE
   *   b: compact 2D array of size TILE x TILE
   *   out: compact 2D array of size TILE x TILE to write to
   */

  a = (const float*)__builtin_assume_aligned(a, TILE * ELEM_SIZE);
  b = (const float*)__builtin_assume_aligned(b, TILE * ELEM_SIZE);
  out = (float*)__builtin_assume_aligned(out, TILE * ELEM_SIZE);

  for(size_t i=0;i<TILE;i++) {
    for(size_t k=0;k<TILE;k++){
      float tmp = 0.0;
      for(size_t j=0;j<TILE;j++){
        tmp += a[i * TILE + j] * b[j * TILE + k];
      }
      out[i * TILE + k] += tmp;
    }
  }
}

void MatmulTiled(const AlignedArray& a, const AlignedArray& b, AlignedArray* out, uint32_t m,
                 uint32_t n, uint32_t p) {
  /**
   * Matrix multiplication on tiled representations of array.  In this setting, a, b, and out
   * are all *4D* compact arrays of the appropriate size, e.g. a is an array of size
   *   a[m/TILE][n/TILE][TILE][TILE]
   * You should do the multiplication tile-by-tile to improve performance of the array (i.e., this
   * function should call `AlignedDot()` implemented above).
   *
   * Note that this function will only be called when m, n, p are all multiples of TILE, so you can
   * assume that this division happens without any remainder.
   *
   * Args:
   *   a: compact 4D array of size m/TILE x n/TILE x TILE x TILE
   *   b: compact 4D array of size n/TILE x p/TILE x TILE x TILE
   *   out: compact 4D array of size m/TILE x p/TILE x TILE x TILE to write to
   *   m: rows of a / out
   *   n: columns of a / rows of b
   *   p: columns of b / out
   *
   */
  for(size_t i=0;i<m/TILE;i++) {
    for(size_t k=0;k<p/TILE;k++){
      for(size_t q1=0;q1<TILE;q1++){
        for(size_t q2=0;q2<TILE;q2++){
          out->ptr[i * p * TILE + k * TILE * TILE + q1 * TILE + q2] = 0.0;
        }
      }
      for(size_t j=0;j<n/TILE;j++){
        AlignedDot(&a.ptr[i * n * TILE + j * TILE * TILE], &b.ptr[j * p * TILE + k * TILE * TILE], &out->ptr[i * p * TILE + k * TILE * TILE]);
      }
    }
  }
}

void ReduceMax(const AlignedArray& a, AlignedArray* out, size_t reduce_size) {
  /**
   * Reduce by taking maximum over `reduce_size` contiguous blocks.
   *
   * Args:
   *   a: compact array of size a.size = out.size * reduce_size to reduce over
   *   out: compact array to write into
   *   reduce_size: size of the dimension to reduce over
   */
  for(size_t i=0;i<out->size;i++) {
    scalar_t tmp = a.ptr[i * reduce_size];
    for(size_t j=1;j< reduce_size;j++){
      if (a.ptr[i * reduce_size + j] > tmp) {
        tmp = a.ptr[i * reduce_size + j];
      }
    }
    out->ptr[i] = tmp;
  }
}

void ReduceSum(const AlignedArray& a, AlignedArray* out, size_t reduce_size) {
  /**
   * Reduce by taking sum over `reduce_size` contiguous blocks.
   *
   * Args:
   *   a: compact array of size a.size = out.size * reduce_size to reduce over
   *   out: compact array to write into
   *   reduce_size: size of the dimension to reduce over
   */
  for(size_t i=0;i<out->size;i++) {
    scalar_t tmp = 0.0;
    for(size_t j=0;j< reduce_size;j++){
      tmp += a.ptr[i * reduce_size + j];
    }
    out->ptr[i] = tmp;
  }
}

}  // namespace cpu
}  // namespace needle

PYBIND11_MODULE(ndarray_backend_cpu, m) {
  namespace py = pybind11;
  using namespace needle;
  using namespace cpu;

  m.attr("__device_name__") = "cpu";
  m.attr("__tile_size__") = TILE;

  py::class_<AlignedArray>(m, "Array")
      .def(py::init<size_t>(), py::return_value_policy::take_ownership)
      .def("ptr", &AlignedArray::ptr_as_int)
      .def_readonly("size", &AlignedArray::size);

  // return numpy array (with copying for simplicity, otherwise garbage
  // collection is a pain)
  m.def("to_numpy", [](const AlignedArray& a, std::vector<size_t> shape,
                       std::vector<size_t> strides, size_t offset) {
    std::vector<size_t> numpy_strides = strides;
    std::transform(numpy_strides.begin(), numpy_strides.end(), numpy_strides.begin(),
                   [](size_t& c) { return c * ELEM_SIZE; });
    return py::array_t<scalar_t>(shape, numpy_strides, a.ptr + offset);
  });

  // convert from numpy (with copying)
  m.def("from_numpy", [](py::array_t<scalar_t> a, AlignedArray* out) {
    std::memcpy(out->ptr, a.request().ptr, out->size * ELEM_SIZE);
  });

  m.def("fill", Fill);
  m.def("compact", Compact);
  m.def("ewise_setitem", EwiseSetitem);
  m.def("scalar_setitem", ScalarSetitem);
  m.def("ewise_add", EwiseAdd);
  m.def("scalar_add", ScalarAdd);

  m.def("ewise_mul", EwiseMul);
  m.def("scalar_mul", ScalarMul);
  m.def("ewise_div", EwiseDiv);
  m.def("scalar_div", ScalarDiv);
  m.def("scalar_power", ScalarPower);

  m.def("ewise_maximum", EwiseMaximum);
  m.def("scalar_maximum", ScalarMaximum);
  m.def("ewise_eq", EwiseEq);
  m.def("scalar_eq", ScalarEq);
  m.def("ewise_ge", EwiseGe);
  m.def("scalar_ge", ScalarGe);

  m.def("ewise_log", EwiseLog);
  m.def("ewise_exp", EwiseExp);
  m.def("ewise_tanh", EwiseTanh);

  m.def("matmul", Matmul);
  m.def("matmul_tiled", MatmulTiled);

  m.def("reduce_max", ReduceMax);
  m.def("reduce_sum", ReduceSum);
}
