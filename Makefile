.PHONY: lib, pybind, clean, format, all

all: lib


lib:
	@mkdir -p build
	@cd build; cmake ..
	@cd build; $(MAKE)

format:
	python3 -m black .
	clang-format -i src/*.cc src/*.cu

clean:
	rm -rf build python/needle/backend_ndarray/ndarray_backend*.so
	rm -rf python/needle/__pycache__
	rm -rf python/needle/backend_ndarray/__pycache__
	rm -rf tests/__pycache__

