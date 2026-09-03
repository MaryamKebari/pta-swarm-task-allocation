.PHONY: build clean smoke test figures verify checksums

PYTHON ?= python3

build:
	$(MAKE) -C simulator/src sim

clean:
	$(MAKE) -C simulator/src clean
	rm -rf build

smoke: build
	$(PYTHON) scripts/run_smoke.py

test:
	$(PYTHON) -m unittest discover -s tests -v

figures:
	$(PYTHON) scripts/make_figures.py

verify:
	$(PYTHON) scripts/verify_repository.py

checksums:
	$(PYTHON) scripts/write_checksums.py
