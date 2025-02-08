.PHONY: all pixelwall others

all: pixelwall others

pixelwall:
	$(MAKE) -C pixelwall

others:
	$(MAKE) -C others

clean:
	$(MAKE) -C pixelwall clean
	$(MAKE) -C others clean
