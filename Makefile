.PHONY: build clean smoke campaign-smoke tuning-smoke test figures verify paper-audit

PYTHON ?= python3

build:
	$(MAKE) -C simulator/src sim

clean:
	$(MAKE) -C simulator/src clean
	rm -rf build

smoke: build
	$(PYTHON) scripts/run_smoke.py

campaign-smoke: build
	$(PYTHON) experiments/run_campaign.py clean --smoke --overwrite --output build/campaign-smoke/per_run_results.csv

tuning-smoke: build
	$(PYTHON) experiments/tune_reference.py --smoke --output build/tuning-smoke

test:
	$(PYTHON) -m unittest discover -s tests -v

figures:
	$(PYTHON) scripts/make_figures.py

verify:
	$(PYTHON) scripts/verify_repository.py

paper-audit: build smoke test verify figures
	@echo "PASS: paper code, processed data, and figures are consistent"
