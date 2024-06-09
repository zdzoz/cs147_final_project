default: help

help:
	@echo "Enter a valid target"
	@echo "  - upload"
	@echo "  - monitor"
	@echo "  - generate-commands"

upload:
	@pio run -t upload

monitor:
	@pio device monitor

generate-commands:
	@pio run -t compiledb
