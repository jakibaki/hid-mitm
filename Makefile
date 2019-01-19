MODULES := hid_mitm

SUBFOLDERS := libstratosphere $(MODULES)

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBFOLDERS)

$(SUBFOLDERS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

$(MODULES): libstratosphere

.PHONY: $(TOPTARGETS) $(SUBFOLDERS)
