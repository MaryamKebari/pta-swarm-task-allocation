.PHONY: build clean smoke campaign-smoke tuning-smoke test figures verify paper-audit

PYTHON ?= python3

build:
	$(MAKE) -C simulator/src sim

clean:
	$(MAKE) -C simulator/src clean
	rm -rf build

smoke: build
	$(PYTHON) experiments/smoke.py

campaign-smoke: build
	$(PYTHON) experiments/run.py allocation --smoke --overwrite --output build/campaign-smoke/per_run_results.csv

tuning-smoke: build
	$(PYTHON) experiments/tune.py --smoke --output build/tuning-smoke

test:
	$(PYTHON) -m unittest discover -s tests -v

figures:
	$(PYTHON) analysis/figures.py

verify:
	$(PYTHON) tests/verify_repository.py

paper-audit: build smoke test verify figures
	@echo "PASS: paper code, processed data, and figures are consistent"
